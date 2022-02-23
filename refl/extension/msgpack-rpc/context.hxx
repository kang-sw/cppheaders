/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2022. Seungwoo Kang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * project home: https://github.com/perfkitpp
 ******************************************************************************/

#pragma once
#include <list>
#include <map>
#include <set>
#include <utility>

#include "../../../functional.hxx"
#include "../../../helper/exception.hxx"
#include "../../../thread/event_wait.hxx"
#include "../../../thread/locked.hxx"
#include "../../../timer.hxx"
#include "../../__namespace__"
#include "../../archive/msgpack-reader.hxx"
#include "../../archive/msgpack-writer.hxx"
#include "../../detail/object_core.hxx"
#include "../../detail/primitives.hxx"

namespace CPPHEADERS_NS_::msgpack::rpc {
using namespace archive::msgpack;

#ifndef CPPHEADERS_MSGPACK_RPC_STREAMBUF_BUFFERSIZE
#    define CPPHEADERS_MSGPACK_RPC_STREAMBUF_BUFFERSIZE 384  // some magic number
#endif

CPPH_DECLARE_EXCEPTION(exception, std::exception);
CPPH_DECLARE_EXCEPTION(invalid_connection, exception);

namespace detail {
CPPH_DECLARE_EXCEPTION(rpc_handler_error, exception);
CPPH_DECLARE_EXCEPTION(rpc_handler_missing_parameter, rpc_handler_error);
CPPH_DECLARE_EXCEPTION(rpc_handler_fatal_state, rpc_handler_error);
}  // namespace detail

namespace detail {
class session;
}

class context;

/**
 * Defines service information
 */
class service_info
{
   public:
    class if_service_handler
    {
        size_t _n_params = 0;

       public:
        explicit if_service_handler(size_t n_params) noexcept : _n_params(n_params) {}

        virtual ~if_service_handler() = 0;

        //! invoke with given parameters.
        //! @return error message if invocation failed
        virtual void invoke(reader&) = 0;

        //! retrive return value
        virtual void retrieve(writer&) = 0;

        //! number of parameters
        size_t num_params() const noexcept { return _n_params; }
    };

    using handler_table_type = std::map<std::string, std::unique_ptr<if_service_handler>, std::less<>>;

   private:
    handler_table_type _handlers;

   public:
    /**
     * Optimized version of serve(). it lets handler reuse return value buffer.
     */
    template <typename Str_, typename RetVal_, typename... Params_>
    service_info& serve_2(Str_&& method_name, function<void(RetVal_*, Params_...)> handler)
    {
        struct service_handler : if_service_handler
        {
            using handler_type = decltype(handler);
            using return_type  = std::conditional_t<std::is_void_v<RetVal_>, nullptr_t, RetVal_>;
            using tuple_type   = std::tuple<Params_...>;

           public:
            handler_type _handler;
            tuple_type _params;
            return_type _rval;

           public:
            explicit service_handler(handler_type&& h) noexcept
                    : if_service_handler(sizeof...(Params_)),
                      _handler(std::move(h)) {}

            void invoke(reader& reader) override
            {
                std::apply(
                        [&](auto&... params) {
                            ((reader >> params), ...);

                            if constexpr (std::is_void_v<RetVal_>)
                                _handler(nullptr, params...);
                            else
                                _handler(&_rval, params...);
                        },
                        _params);
            }

            void retrieve(writer& writer) override
            {
                if constexpr (std::is_void_v<RetVal_>)
                    writer << nullptr;
                else
                    writer << _rval;
            }
        };

        auto [it, is_new] = _handlers.try_emplace(
                std::forward<Str_>(method_name),
                std::make_unique<service_handler>(std::move(handler)));

        if (not is_new)
            throw std::logic_error{"Method name may not duplicate!"};

        return *this;
    }

    /**
     * Serve RPC service. Does not distinguish notify/request handler. If client rpc mode was
     *  notify, return value of handler will silently be discarded regardless of its return
     *  type.
     *
     * @tparam RetVal_ Must be declared as refl object.
     * @tparam Params_ Must be declared as refl object.
     * @param method_name Name of method to serve
     * @param handler RPC handler. Uses cpph::function to accept move-only signatures
     */
    template <typename Str_, typename RetVal_, typename... Params_>
    service_info& serve(Str_&& method_name, function<RetVal_(Params_...)> handler)
    {
        this->serve_2(
                std::forward<Str_>(method_name),
                [_handler = std::move(handler)]  //
                (RetVal_ * buffer, Params_ && ... args) mutable {
                    if constexpr (std::is_void_v<RetVal_>)
                        _handler(std::forward<Params_>(args)...);
                    else
                        *buffer = _handler(std::forward<Params_>(args)...);
                });

        return *this;
    }

   public:
    auto const& _services_() const noexcept { return _handlers; }
};

class connection_streambuf : std::streambuf
{
    detail::session* _owner = {};

   public:
    ~connection_streambuf() override = 0;

    /**
     * Call on read new data
     */
    void notify();

    /**
     * Start communication. Before this call, call to notify() cause crash.
     *
     * @throw invalid_connection on connection invalidated during launch
     */
    virtual void launch() = 0;

    /**
     * Called when session disconnected by parsing error or something else.
     *
     * Lets internal connection to refresh its connection.
     *
     * e.g. Notify connection manager to reconnect this session.
     */
    virtual void reconnect() = 0;

   public:
    void _init_(detail::session* sess) { _owner = sess, launch(); }
};

enum class rpc_status
{
    okay    = 0,
    waiting = 1,

    timeout = -10,

    unknown_error       = -1,
    internal_error      = -2,
    invalid_parameter   = -3,
    invalid_return_type = -4,

    dead_peer = -100,
};

namespace detail {
#if 0  // TODO: is this necessary?
class connection_streambuf : public std::streambuf
{
    if_connection* _conn;
    char _ibuf[CPPHEADERS_MSGPACK_RPC_STREAMBUF_BUFFERSIZE] = {};
    char _obuf[CPPHEADERS_MSGPACK_RPC_STREAMBUF_BUFFERSIZE] = {};

   public:
    explicit connection_streambuf(if_connection* conn) : _conn{conn}
    {
        setp(_obuf, *(&_obuf + 1));
    }

   protected:
    int sync() override
    {
        _flush();
        return 0;
    }

    int_type overflow(int_type int_type) override
    {
        _flush();
        return sputc(traits_type::to_char_type(int_type));
    }

    int_type underflow() override
    {
        auto n_read = _conn->read({_ibuf, sizeof _ibuf});
        setg(_ibuf, _ibuf, _ibuf + n_read);
        return traits_type::to_int_type(_ibuf[0]);
    }

    std::streamsize xsgetn(char* ptr, std::streamsize count) override
    {
        size_t retval = 0;

        // Consume remaining gbuf first
        {
            auto navail = std::min(in_avail(), count);
            traits_type::copy(ptr, gptr(), navail);
            gbump(navail);
            count -= navail;
            retval += navail;
            ptr += navail;
        }

        // If remaining buffer size is relatively small, use original functionality which
        //  buffers incoming bytes first. This lets next read be called
        if (count < sizeof _ibuf)
            return retval + std::basic_streambuf<char>::xsgetn(ptr, count);

        // In case of big buffer, which cannot be covered by sizeof(_ibuf), whill directly
        //  request read without buffering.
        while (0 < count)
        {
            // In case of relatively large buffer, retrieve
            auto n_read = _conn->read({ptr, size_t(count)});
            count -= n_read, ptr += n_read, retval += n_read;
        }

        return retval;
    }

    std::streamsize xsputn(const char* ptr, std::streamsize count) override
    {
        auto pnwrite = pptr() - pbase();

        // If size of bytes to write is relatively small, just use default functionality
        if (pnwrite + count < (sizeof(_obuf) * 3 / 2))
            return std::streambuf::xsputn(ptr, count);

        // Otherwise, first flush buffer, and then write rest of bytes
        if (pnwrite)
        {
            // Flush output buffer
            _conn->write({pbase(), size_t(pnwrite)});
            setp(_obuf, *(&_obuf + 1));
        }

        _conn->write({ptr, size_t(count)});

        // Always return requested value. If there's any error, write() will just throw.
        return count;
    }

   private:
    void _flush()
    {
        _conn->write({pbase(), size_t(pptr() - pbase())});
        setp(_obuf, *(&_obuf + 1));
    }
};
#endif

enum class rpc_type
{
    request = 0,
    reply   = 1,
    notify  = 2,
};

inline std::string_view to_string(rpc_status s)
{
    static std::map<rpc_status, std::string const, std::less<>> etos{
            {rpc_status::okay, "OKAY"},
            {rpc_status::waiting, "WAITING"},
            {rpc_status::timeout, "ERROR_TIMEOUT"},
            {rpc_status::unknown_error, "UNKNOWN"},
            {rpc_status::internal_error, "ERROR_INTERNAL"},
            {rpc_status::invalid_parameter, "ERROR_INVALID_PARAMETER"},
            {rpc_status::invalid_return_type, "ERROR_INVALID_RETURN_TYPE"},
    };

    auto p = find_ptr(etos, s);
    return p ? std::string_view(p->second) : std::string_view("UNKNOWN");
}

inline auto from_string(std::string_view s)
{
    static std::map<std::string, rpc_status, std::less<>> stoe{
            {"OKAY", rpc_status::okay},
            {"WAITING", rpc_status::waiting},
            {"ERROR_TIMEOUT", rpc_status::timeout},
            {"UNKOWN", rpc_status::unknown_error},
            {"ERROR_INTERNAL", rpc_status::internal_error},
            {"ERROR_INVALID_PARAMETER", rpc_status::invalid_parameter},
            {"ERROR_INVALID_RETURN_TYPE", rpc_status::invalid_return_type},
    };

    auto p = find_ptr(stoe, s);
    return p ? p->second : rpc_status::unknown_error;
}

/**
 * Indicates single connection
 *
 * - Write request may occur in multiple threads, thus it should be protected by mutex
 * - Read request may occur only one thread, thus it may not be protected.
 */
class session : public std::enable_shared_from_this<session>
{
   public:
    struct config
    {
        bool use_integer_key              = true;
        std::chrono::microseconds timeout = {};
    };

   private:
    struct request_info
    {
        function<void(reader*)> promise;
        volatile rpc_status status = rpc_status::waiting;
    };

   private:
    context* _owner = {};
    config _conf;

    // A session will automatically be expired if connection is destroyed.
    std::unique_ptr<connection_streambuf> _conn;

    // msgpack stream reader/writers
    reader _reader{nullptr, 16};
    writer _writer{nullptr, 16};

    // Writing operation is protected.
    // - When reply to RPC
    // - When send rpc request
    spinlock _write_lock;
    volatile int _msgid_gen = 0;
    std::string _method_name_buf;

    // Check function is waiting for awake.
    std::atomic_bool _waiting;

    // RPC reply table. Old ones may set timeout exceptions
    std::map<int, request_info> _requests;
    spinlock _rpc_lock;

    // Recv event wait
    thread::event_wait _rpc_notify;

    // Check if pending kill
    std::atomic_bool _pending_kill = false;

   public:
    session(config const& conf, std::unique_ptr<connection_streambuf> conn)
            : _conf(conf), _conn(std::move(conn)) {}

   public:
    template <typename RetVal_, typename... Params_>
    auto rpc_send(RetVal_* result, std::string_view method, Params_&&... params)
    {
        decltype(_requests)::iterator request;
        int msgid;

        // create reply slot
        {
            lock_guard lc{_rpc_lock};
            msgid   = ++_msgid_gen;
            request = _requests.try_emplace(msgid).first;

            std::atomic_bool ready = false;

            // This will be invoked when return value from remote is ready, and will directly
            //  deserialize the result into provided argument.
            request->second.promise =
                    [&](reader* rd) mutable {
                        if constexpr (std::is_same_v<RetVal_, void>)
                            *rd >> nullptr;  // skip
                        else
                            *rd >> *result;
                    };
        }

        // send message
        {
            lock_guard lc{_write_lock};

            _writer.array_push(4);
            {
                _writer << rpc_type::request;
                _writer << msgid;
                _writer << method;

                _writer.array_push(sizeof...(params));
                ((_writer << std::forward<Params_>(params)), ...);
                _writer.array_pop();
            }
            _writer.array_pop();
            _writer.flush();
        }

        return request;
    }

    auto rpc_wait(decltype(_requests)::iterator request)
    {
        // wait for reply
        auto wait_fn = [&] { return request->second.status != rpc_status::waiting; };
        auto ready   = _rpc_notify.wait_for(_conf.timeout, wait_fn);

        // timeout
        auto errc = request->second.status;

        {
            lock_guard lc{_rpc_lock};
            _requests.erase(request);
        }

        return ready ? errc : rpc_status::timeout;
    }

    template <typename... Params_>
    void notify(std::string_view method, Params_&&... params)
    {
        lock_guard lc{_write_lock};

        _writer.array_push(3);
        {
            _writer << rpc_type::notify
                    << method;

            _writer.array_push(sizeof...(params));
            ((_writer << std::forward<Params_>(params)), ...);
            _writer.array_pop();
        }
        _writer.array_pop();
        _writer.flush();
    }

    void wakeup();

    auto lock_write() { return std::unique_lock{_write_lock}; }
    auto try_lock_write() { return std::unique_lock{_write_lock, std::try_to_lock}; }

    bool pending_kill() const noexcept { return _pending_kill; }

   private:
    void _wakeup_func()
    {
        try
        {
            // Always entered by single thread.

            // Assume data is ready to be read.
            auto key = _reader.begin_object();

            rpc_type type;

            switch (_reader >> type, type)
            {
                case rpc_type::request: _handle_request(); break;
                case rpc_type::notify: _handle_notify(); break;

                case rpc_type::reply:
                {
                    int msgid = -1;
                    _reader >> msgid;

                    //  1. find corresponding reply to msgid
                    //  2. fill promise with value or exception
                    auto lc{std::lock_guard{_rpc_lock}};
                    if (auto preq = find_ptr(_requests, msgid))
                    {
                        if (_reader.is_null_next())  // no error
                        {
                            _reader >> nullptr;

                            try
                            {
                                preq->second.promise(&_reader);
                            }
                            catch (std::exception& e)
                            {
                                // on failed to parse result, refresh connection
                                preq->second.status = rpc_status::internal_error;
                                throw detail::rpc_handler_fatal_state{};
                            }
                        }
                        else
                        {
                            std::string errmsg;
                            _reader >> errmsg;
                            _reader >> nullptr;  // skip payload

                            preq->second.status = from_string(errmsg);
                        }

                        // Wake up all waiting candidates.
                        _rpc_notify.notify_all();
                    }

                    break;
                }
            }

            _reader.end_object(key);

            // waiting for next input.
            _waiting = true;
        }
        catch (detail::rpc_handler_fatal_state&)
        {
            // If internal state irreversible, connection will be refreshed.
            _refresh();
        }
        catch (invalid_connection&)
        {
            _erase_self();
        }
    }

    void _handle_request()
    {
        //  1. read msgid
        //  2. read method name
        //  3. invoke handler
        //  4. lock write / retrieve input
        int msgid = {};
        _reader >> msgid;
        _reader >> _method_name_buf;

        if (auto pair = find_ptr(_get_services(), _method_name_buf))
        {
            auto return_null = [&] { _writer << nullptr; };
            auto reply =
                    [&](auto&& a, auto&& bn) {
                        std::lock_guard lc{_write_lock};
                        _writer.array_push(4);
                        {
                            _writer << rpc_type::reply;
                            _writer << msgid;
                            _writer << a;
                            bn();
                        }
                        _writer.array_pop();
                        _writer.flush();
                    };

            auto& srv = pair->second;
            auto ctx  = _reader.begin_array();

            if (_reader.elem_left() < srv->num_params())
            {
                // number of parameters are insufficient.
                reply(to_string(rpc_status::invalid_parameter), return_null);
            }
            else
            {
                try
                {
                    srv->invoke(_reader);

                    // send reply
                    reply(nullptr, [&] { srv->retrieve(_writer); });
                }
                catch (type_mismatch_exception&)
                {
                    reply(to_string(rpc_status::invalid_parameter), return_null);
                }
                catch (archive::error::archive_exception&)
                {
                    throw detail::rpc_handler_fatal_state{};
                }
            }

            _reader.end_array(ctx);
        }
        else
        {
            _reader >> nullptr;
        }
    }

    void _handle_notify()
    {
        //  1. read method name
        //  2. invoke handler

        int msgid = {};
        _reader >> msgid;
        _reader >> _method_name_buf;

        if (auto pair = find_ptr(_get_services(), _method_name_buf))
        {
            auto& srv = pair->second;
            auto ctx  = _reader.begin_array();

            if (_reader.elem_left() >= srv->num_params())
            {
                try
                {
                    srv->invoke(_reader);
                }
                catch (type_mismatch_exception&)
                {
                    ;
                }
                catch (archive::error::archive_exception&)
                {
                    throw detail::rpc_handler_fatal_state{};
                }
            }
            else
            {
                ;  // errnous situation ... as this is notify, just ignore.
            }

            _reader.end_array(ctx);
        }
        else
        {
            _reader >> nullptr;
        }
    }

    service_info::handler_table_type const& _get_services() const;

    void _erase_self();

    void _refresh()
    {
        _writer.clear();
        _reader.clear();
        _waiting = true;
        _conn->reconnect();
    }
};
}  // namespace detail

using session_config = detail::session::config;

class context
{
    friend class detail::session;

    using session_ptr  = std::shared_ptr<detail::session>;
    using session_wptr = std::weak_ptr<detail::session>;

   public:
    using dispatch_function = function<void(function<void()>)>;

   private:
    // Defined services
    service_info _service;

    // List of created sessions.
    std::set<session_ptr, std::owner_less<>> _session_sources;
    std::list<session_wptr> _sessions;

    thread::event_wait _session_notify;
    dispatch_function _dispatch;

   public:
    std::chrono::milliseconds global_timeout{60'000};

   public:
    /**
     * Create new context, with appropriate dispatcher function.
     */
    explicit context(dispatch_function dispatcher = [](auto&& fn) { fn(); })
            : _dispatch(std::move(dispatcher)) {}

    /**
     * Copying/Moving object address is not permitted, as sessions
     */
    context& operator=(context const&) = delete;
    context& operator=(context&&) noexcept = delete;
    context(context const&)                = delete;
    context(context&&) noexcept            = delete;

    /**
     * Create new context with given service information.
     * Once service is registered, it becomes read-only.
     *
     * @param service
     */
    explicit context(service_info service) noexcept : _service(std::move(service)) {}

    /**
     * If context is created without service information,
     */
    context() noexcept = default;

    /**
     * Call RPC function. Will be load-balanced automatically.
     *
     * @tparam RetVal_
     * @tparam Params_
     * @param retval receive output to this memory
     * @param params
     * @return
     */
    template <typename RetVal_, typename... Params_>
    auto rpc(RetVal_* retval, std::string_view method, Params_&&... params) -> rpc_status
    {
        try
        {
            auto session = _checkout();
            if (not session) { return rpc_status::timeout; }

            auto request = session->rpc_send(retval, method, std::forward<Params_>(params)...);
            auto result  = session->rpc_wait(request);

            _checkin(std::move(session));
            return result;
        }
        catch (invalid_connection& e)
        {
            // do nothing, session will be disposed automatically by disposing pointer.
            return rpc_status::dead_peer;
        }
    }

    /**
     * Notify single session
     */
    template <typename... Params_>
    void notify(std::string_view method, Params_&&... params)
    {
        try
        {
            using namespace std::chrono_literals;

            auto session = _checkout(false);
            if (not session) { return; }

            session->notify(method, std::forward<Params_>(params)...);
            _checkin(std::move(session));
        }
        catch (invalid_connection& e)
        {
            // do nothing, session will be disposed automatically by disposing pointer.
        }
    }

    /**
     * Notify all sessions
     */
    template <typename... Params_>
    void notify_all(std::string_view method, Params_&&... params)
    {
        std::vector<session_ptr> all;

        _session_notify.critical_section(
                [&] {
                    all.reserve(_sessions.size());

                    for (auto& wp : _sessions)
                        if (auto sp = _impl_checkout(wp)) { all.emplace_back(std::move(sp)); }
                });

        for (auto& sp : all)
        {
            try
            {
                sp->notify(method, std::forward<Params_>(params)...);
                _checkin(std::move(sp));
            }
            catch (invalid_connection&)
            {
                ;  // do nothing, let it disposed
            }
        }
    }

    /**
     * Create new session with given connection type.
     * Lifecycle of connection must be managed outside of class boundary.
     *
     * @tparam Conn_
     * @tparam Args_
     * @param args
     * @return shared reference to
     */
    template <typename Conn_, typename... Args_,
              typename = std::enable_if_t<std::is_base_of_v<connection_streambuf, Conn_>>>
    void create_session(session_config const& conf, Args_&&... args)
    {
        // put created session reference to sessions_source and sessions both.
        auto connection = std::make_unique<Conn_>(std::forward<Args_>(args)...);
        auto session    = std::make_shared<detail::session>(conf, std::move(connection));

        // notify session creation, for rpc requests which is waiting for
        //  any valid session.
        _session_notify.notify_all(
                [&] {
                    _sessions.push_back(session);
                    _session_sources.emplace(std::move(session));
                });
    }

   protected:
    void dispatch(function<void()> message) { _dispatch(std::move(message)); }

   private:
    void _dispose_session(detail::session* ptr) {}

    session_ptr _checkout(bool wait = true)
    {
        using namespace std::chrono_literals;

        session_ptr session = {};
        auto predicate =
                [&] {
                    // If there's no sessions active ...
                    for (; not _sessions.empty();)
                    {
                        auto ptr = std::move(_sessions.front());
                        _sessions.pop_front();

                        if ((session = _impl_checkout(ptr)) == nullptr)
                            continue;  // dispose wptr

                        // push weak pointer back for round-robin load balancing
                        _sessions.push_back(ptr);

                        return true;
                    }

                    return false;
                };

        if (wait)
            _session_notify.wait_for(global_timeout, predicate);
        else
            _session_notify.critical_section(predicate);

        return session;
    }

    session_ptr _impl_checkout(session_wptr const& ptr)
    {
        session_ptr session = {};

        auto source = _session_sources.find(ptr);
        if (source != _session_sources.end())
        {  // if given session is never occupied by any other context ..
            session = *source;
            _session_sources.erase(source);
        }
        else if ((session = ptr.lock()) == nullptr)
        {
            // given session seems expired ...
            // do nothing, as given pointer already removed from sessions ...
            ;
        }

        return session;
    }

    void _checkin(session_ptr&& ptr)
    {
        _session_notify.critical_section(
                [&] {
                    // if this isn't unique reference, just dispose.
                    if (ptr.use_count() > 1) { return; }

                    // if pointer is pending kill, do not return this to sources buffer.
                    // which will dispose given session, which was given by move operation.
                    if (ptr->pending_kill()) { return; }

                    // otherwise, return it to source buffer to keep object valid
                    _session_sources.insert(std::move(ptr));
                });
    }
};

}  // namespace CPPHEADERS_NS_::msgpack::rpc

//
// __CPP__
//
namespace CPPHEADERS_NS_::msgpack::rpc {

inline void connection_streambuf::notify() { _owner->wakeup(); }

namespace detail {

inline void session::wakeup()
{
    // do nothing if not waiting.
    if (not _waiting.exchange(false)) { return; }

    _owner->dispatch(bind_front_weak(weak_from_this(), &session::_wakeup_func, this));
}

inline service_info::handler_table_type const& session::_get_services() const
{
    return _owner->_service._services_();
}

inline void session::_erase_self()
{
    _conn.reset();

    _owner->_session_notify.critical_section(
            [this] {
                _pending_kill = true;
                auto wp       = weak_from_this();

                // If any other write request is alive, let it dispose this.
                if (wp.use_count() != 0) { return; }

                // Otherwise, dispose this
                auto iter = _owner->_session_sources.find(wp);
                _owner->_session_sources.erase(iter);
            });
}

}  // namespace detail
}  // namespace CPPHEADERS_NS_::msgpack::rpc