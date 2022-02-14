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
#include <future>
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
    enum class invoke_result
    {
        ok    = 0,
        error = -1,  // reconnection required
    };

   private:
    class service_handler
    {
        size_t _n_params = 0;

       public:
        virtual ~service_handler() = 0;

        //! invoke with given parameters.
        //! @return error message if invocation failed
        virtual auto invoke(reader&) -> invoke_result = 0;

        //! retrive return value
        virtual void retrieve(writer&) = 0;

        //! number of parameters
        size_t num_params() const noexcept { return _n_params; }
    };

   private:
    std::map<std::string, std::unique_ptr<service_handler>, std::less<>> _handlers;

   public:
    /**
     * Optimized version of serve(). it lets handler reuse return value buffer.
     */
    template <typename Str_, typename RetVal_, typename... Params_>
    service_info& serve_2(Str_&& method_name, function<void(RetVal_*, Params_...)> handler)
    {
        // TODO
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
                    *buffer = _handler(std::forward<Params_>(args)...);
                });

        return *this;
    }

   public:
    auto const& _services_() const noexcept { return _handlers; }
};

class if_connection
{
    detail::session* _owner = {};

   public:
    virtual ~if_connection() = 0;

    /**
     * Wakeup this connection for incoming data stream
     *
     * If connection disabled, calling this and throwing error on next read/write will trigger
     *  disposing connection
     */
    void wakeup();

    /**
     * Initialize this connection.
     *
     * Calling wakeup() is unsafe before this function call.
     *
     * @throw invalid_connection on connection invalidated during initialize
     */
    virtual void initialize() = 0;

    /**
     * If called, next data receive must call wakeup()
     *
     * It may call wakeup immediately, if there's any data available.
     */
    virtual void begin_waiting() = 0;

    /**
     * Recv n bytes from buffer. Waits infinitely untill read all data.
     *
     * @throw invalid_connection on connection invalidated
     */
    virtual void read(array_view<void> buffer) = 0;

    /**
     * Write n bytes to buffer
     *
     * @throw invalid_connection on connection invalidated
     */
    virtual void write(array_view<void const> payload) = 0;

    /**
     * Called when session disconnected by parsing error or something else.
     *
     * Lets internal connection to refresh its connection.
     *
     * e.g. Notify connection manager to reconnect this session.
     */
    virtual void reconnect() = 0;

   public:
    void _init_(detail::session* sess) { _owner = sess, initialize(); }
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
class connection_streambuf : public std::streambuf
{
   public:
    if_connection* _conn;

   protected:
    int_type overflow(int_type int_type) override
    {
        // TODO
        return basic_streambuf::overflow(int_type);
    }
    int_type underflow() override
    {
        // TODO
        return basic_streambuf::underflow();
    }
    std::streamsize xsgetn(char* _Ptr, std::streamsize _Count) override
    {
        // TODO
        return basic_streambuf::xsgetn(_Ptr, _Count);
    }
    std::streamsize xsputn(const char* _Ptr, std::streamsize _Count) override
    {
        // TODO
        return basic_streambuf::xsputn(_Ptr, _Count);
    }
};

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
class session
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
    std::shared_ptr<if_connection> _conn;

    // msgpack stream reader/writers
    detail::connection_streambuf _buffer;
    reader _reader{&_buffer, 16};
    writer _writer{&_buffer, 16};

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
        }
        catch (detail::rpc_handler_fatal_state&)
        {
            // on parsing error, refresh internal connection.
            _refresh();
        }
        catch (invalid_connection&)
        {
            _conn.reset();
            _erase_self();
        }
    }

    void _handle_request();
    void _handle_notify();
    void _erase_self();

    void _refresh()
    {
        _writer.clear();
        _reader.clear();
        _waiting = true;
        _conn->reconnect();
        _conn->begin_waiting();
    }
};
}  // namespace detail

using session_config = detail::session::config;

class context
{
    friend class detail::session;

    using session_ptr  = std::shared_ptr<detail::session>;
    using session_wptr = std::weak_ptr<detail::session>;

   private:
    // Defined services
    service_info _service;

    // List of created sessions.
    std::set<session_ptr, std::owner_less<>> _session_sources;
    std::list<session_wptr> _sessions;

    thread::event_wait _session_notify;

   public:
    std::chrono::milliseconds global_timeout{60'000};

   public:
    /**
     * For redefining dispatch function ...
     */
    virtual ~context() noexcept = default;

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

            auto result = session->rpc_wait(request);
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
              typename = std::enable_if_t<std::is_base_of_v<if_connection, Conn_>>>
    void create_session(session_config const& conf, Args_&&...)
    {
        // TODO
        // put created session reference to sessions_source and sessions both.

        // notify session creation, for rpc requests which is waiting for
        //  any valid session.
    }

   protected:
    //! Post message for read stream
    virtual void dispatch(function<void()> message) { message(); }

   private:
    void _dispose_session(detail::session* ptr) {}

    session_ptr _checkout(bool wait = true)
    {
        using namespace std::chrono_literals;

        session_ptr session = {};
        auto predicate =
                [&] {
                    if (_sessions.empty()) { return false; }

                    for (;;)
                    {
                        auto ptr = std::move(_sessions.front());
                        _sessions.pop_front();

                        if ((session = _impl_checkout(ptr)) == nullptr)
                            continue;

                        // push weak pointer back for round-robin load balancing
                        _sessions.push_back(ptr);
                    }

                    return true;
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

inline void if_connection::wakeup() { _owner->wakeup(); }

namespace detail {
inline void session::_handle_request()
{
    //  1. read msgid
    //  2. read method name
    //  3. invoke handler
    //  4. lock write / retrieve input
    int msgid = {};
    _reader >> msgid;
    _reader >> _method_name_buf;

    if (auto pair = find_ptr(_owner->_service._services_(), _method_name_buf))
    {
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
                };

        auto& srv = pair->second;
        auto ctx  = _reader.begin_array();

        if (_reader.elem_left() < srv->num_params())
        {
            // number of parameters are insufficient.
            reply(to_string(rpc_status::invalid_parameter), [] {});
        }
        else
        {
            auto r_invoke = srv->invoke(_reader);
            if (r_invoke != service_info::invoke_result::error)
            {
                reply(to_string(rpc_status::internal_error), [] {});
                throw detail::rpc_handler_fatal_state{};
                // as type error is hard to resolve, simply refresh connection status
            }

            // send reply
            reply(nullptr, [&] { srv->retrieve(_writer); });
        }

        _reader.end_array(ctx);
    }
    else
    {
        _reader >> nullptr;
    }
}

inline void session::_handle_notify()
{
    //  1. read method name
    //  2. invoke handler

    int msgid = {};
    _reader >> msgid;
    _reader >> _method_name_buf;

    if (auto pair = find_ptr(_owner->_service._services_(), _method_name_buf))
    {
        auto& srv = pair->second;
        auto ctx  = _reader.begin_array();

        if (_reader.elem_left() >= srv->num_params())
        {
            auto r_invoke = srv->invoke(_reader);
            if (r_invoke != service_info::invoke_result::error)
                throw detail::rpc_handler_fatal_state{};  // type error is hard to resolve.

            // don't need to retrieve result.
        }

        _reader.end_array(ctx);
    }
    else
    {
        _reader >> nullptr;
    }
}

inline void session::wakeup()
{
    if (not _waiting.exchange(false)) { throw std::logic_error{"target was not waiting!"}; }
    _owner->dispatch(bind_front(&session::_wakeup_func, this));
}

}  // namespace detail
}  // namespace CPPHEADERS_NS_::msgpack::rpc