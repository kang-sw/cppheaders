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
#include <any>
#include <deque>
#include <map>
#include <set>
#include <utility>

#include "../../../functional.hxx"
#include "../../../memory/pool.hxx"
#include "../../../thread/event_wait.hxx"
#include "../../../thread/locked.hxx"
#include "../../../timer.hxx"
#include "../../__namespace__"
#include "../../archive/msgpack-reader.hxx"
#include "../../archive/msgpack-writer.hxx"
#include "../../detail/object_core.hxx"
#include "../../detail/primitives.hxx"
#include "defs.hxx"
#include "errors.hxx"
#include "service_info.hxx"

namespace CPPHEADERS_NS_::msgpack::rpc {
using namespace archive::msgpack;

namespace detail {
class session;
}

class context;

/**
 * This is the only class that you have to inherit and implement details.
 *
 * Once connection is invalidated, any call to methods of this class should
 * throw \refitem{invalid_connection} to gently cleanup this session.
 */
class if_connection
{
    std::weak_ptr<detail::session> _owner = {};
    std::string _peer;

   public:
    virtual ~if_connection() = default;

    /**
     * Initialize connection
     *
     * @param peer
     * @param buf
     */
    explicit if_connection(std::string peer) noexcept
            : _peer(std::move(peer)) { _peer.shrink_to_fit(); }

    /**
     * Returns name of the peer
     */
    std::string const& peer() const noexcept { return _peer; }

    /**
     * Returns internal streambuf
     */
    virtual std::streambuf* rdbuf() = 0;

    /**
     * If this is called, start waiting for data asynchronously.
     */
    virtual void begin_wait() = 0;

    /**
     * On waiting state, asynchronous data input notification should call this function.
     */
    void notify_receive();

    /**
     * On disconnection occurred ..
     */
    void notify_disconnect();

    /**
     * Start communication. Before this call, call to notify() cause crash.
     *
     * @throw invalid_connection on connection invalidated during launch
     */
    virtual void launch() = 0;

    /**
     * Force disconnect
     */
    virtual void disconnect() {}

    /**
     * Set timeout
     */
    virtual void set_timeout(std::chrono::microseconds) {}

    /**
     * Get owner weak pointer
     */
    auto owner() const { return _owner; }

   public:
    void _init_(std::weak_ptr<detail::session> sess) { _owner = sess, launch(), begin_wait(); }
};

/**
 * Event monitor
 */
class if_context_monitor
{
   public:
    //! Exception must not propagate!
    virtual void on_new_session(session_profile const&) noexcept {}

    //! Exception must not propagate!
    virtual void on_dispose_session(session_profile const&) noexcept {}
};

namespace detail {

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
            {rpc_status::method_not_exist, "ERROR_METHOD_NOT_EXIST"},
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
            {"ERROR_METHOD_NOT_EXIST", rpc_status::method_not_exist},
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
        std::exception_ptr except;
        volatile rpc_status status = rpc_status::waiting;
    };

   private:
    friend class rpc::context;

    context* _owner = {};
    config _conf;

    // A session will automatically be expired if connection is destroyed.
    std::unique_ptr<if_connection> _conn;

    // Profile for this session
    session_profile _profile;

    // msgpack stream reader/writers
    reader _reader{_conn->rdbuf(), 16};
    writer _writer{_conn->rdbuf(), 16};

    // Writing operation is protected.
    // - When reply to RPC
    // - When send rpc request
    spinlock _write_lock;
    volatile int _msgid_gen = 0;
    std::string _method_name_buf;

    // Check function is waiting for awake.
    std::atomic_bool _waiting{false};

    // RPC reply table. Old ones may set timeout exceptions
    std::map<int, request_info> _requests;

    // Recv event wait
    thread::event_wait _rpc_notify;

    // Check if pending kill
    std::atomic_bool _pending_kill = false;

    // write refcount
    volatile int _refcnt = 0;

    std::weak_ptr<if_context_monitor> _monitor;

   public:
    session(context* owner,
            config const& conf,
            std::unique_ptr<if_connection> conn,
            std::weak_ptr<if_context_monitor> wmonitor) noexcept
            : _owner(owner), _conf(conf), _conn(std::move(conn)), _monitor(std::move(wmonitor))
    {
        if (_conf.timeout.count() == 0) { _conf.timeout = std::chrono::hours{2400}; }

        // Fill profile info
        _profile.peer_name = _conn->peer();
        _conn->set_timeout(conf.timeout);

        // Notify creation to monitor
        if (auto monitor = _monitor.lock()) { monitor->on_new_session(_profile); }
    }

    ~session() noexcept
    {
        if (auto monitor = _monitor.lock()) { monitor->on_dispose_session(_profile); }
    }

   public:
    template <typename RetPtr_, typename... Params_>
    auto rpc_send(RetPtr_ result, std::string_view method, Params_&&... params)
    {
        decltype(_requests)::iterator request;
        int msgid;

        // create reply slot
        _rpc_notify.critical_section([&] {
            msgid     = ++_msgid_gen;
            auto pair = _requests.try_emplace(msgid);
            request   = pair.first;
            assert(pair.second);  // message id should never duplicate

            // This will be invoked when return value from remote is ready, and will directly
            //  deserialize the result into provided argument.
            request->second.promise =
                    [result](reader* rd) {
                        if constexpr (std::is_null_pointer_v<RetPtr_>)
                        {
                            *rd >> nullptr;
                        }
                        else if constexpr (std::is_void_v<std::remove_pointer_t<RetPtr_>>)
                        {
                            *rd >> nullptr;
                        }
                        else
                        {
                            if (result == nullptr)
                                *rd >> nullptr;  // skip
                            else
                                *rd >> *result;
                        }
                    };
        });

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
        auto errc   = request->second.status;
        auto except = std::move(request->second.except);

        _rpc_notify.critical_section([&] {
            _requests.erase(request);
        });

        if (errc == rpc_status::unknown_error && except)
            std::rethrow_exception(except);

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
    void dispose_self()
    {
        _erase_self();
    }

    auto lock_write() { return std::unique_lock{_write_lock}; }
    auto try_lock_write() { return std::unique_lock{_write_lock, std::try_to_lock}; }

    bool pending_kill() const noexcept { return _pending_kill.load(std::memory_order_acquire); }

    void _start_()
    {
        // As weak_from_this cannot be called inside of constructor,
        //  initialization of each sessions must be called explicitly.
        _waiting = true;
        _conn->_init_(weak_from_this());
    }

   private:
    void _wakeup_func()
    {
        // NOTE: This function guaranteed to not be re-entered multiple times on single session
        try
        {
            // Assume data is ready to be read.
            auto key = _reader.begin_array();

            rpc_type type;

            switch (_reader >> type, type)
            {
                case rpc_type::request: _handle_request(); break;
                case rpc_type::notify: _handle_notify(); break;
                case rpc_type::reply: _handle_reply(); break;

                default: throw invalid_connection{};
            }

            _reader.end_array(key);

            // waiting for next input.
            _waiting.store(true, std::memory_order_release);
            _conn->begin_wait();
        }
        catch (detail::rpc_handler_fatal_state&)
        {
            _erase_self();
        }
        catch (invalid_connection&)
        {
            _erase_self();
        }
        catch (archive::error::archive_exception&)
        {
            _erase_self();
        }
    }

    void _handle_reply()
    {
        int msgid = -1;
        _reader >> msgid;

        //  1. find corresponding reply to msgid
        //  2. fill promise with value or exception
        _rpc_notify.notify_all([&] {
            if (auto preq = find_ptr(_requests, msgid))
            {
                rpc_status next_status;

                if (_reader.is_null_next())  // no error
                {
                    _reader >> nullptr;

                    try
                    {
                        preq->second.promise(&_reader);
                        next_status = rpc_status::okay;
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
                    // TODO: Extract 'errmsg' as object and dump to string!
                    std::string errmsg;
                    _reader >> errmsg;
                    _reader >> nullptr;  // skip payload

                    next_status = from_string(errmsg);

                    if (next_status == rpc_status::unknown_error)
                        preq->second.except = std::make_exception_ptr(remote_reply_exception{errmsg});
                }

                // Wake up all waiting candidates.
                preq->second.status = next_status;
            }
            else
            {
                assert(preq == nullptr);
            }
        });
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

        static const auto fn_reply
                = [](decltype(this) self, int msgid, auto&& err, auto&& result) {
                      lock_guard _lc_{self->_write_lock};

                      self->_writer.array_push(4);
                      self->_writer << rpc_type::reply
                                    << msgid
                                    << err
                                    << result;
                      self->_writer.array_pop();
                      self->_writer.flush();
                  };

        if (auto pair = find_ptr(_get_services(), _method_name_buf))
        {
            auto& srv = pair->second;
            auto ctx  = _reader.begin_array();  // begin reading params

            if (_reader.elem_left() < srv->num_params())
            {
                // number of parameters are insufficient.
                fn_reply(this, msgid, to_string(rpc_status::invalid_parameter), nullptr);
            }
            else
            {
                try
                {
                    struct uobj_t
                    {
                        session* self;
                        int msgid;
                    } uobj{this, msgid};

                    srv->invoke(
                            _profile, _reader,
                            [](void* pvself, refl::object_const_view_t data) {
                                auto self  = ((uobj_t*)pvself)->self;
                                auto msgid = ((uobj_t*)pvself)->msgid;

                                fn_reply(self, msgid, nullptr, data);
                            },
                            &uobj);
                }
                catch (remote_handler_exception& ec)
                {
                    fn_reply(this, msgid, ec.view(), nullptr);
                }
                catch (type_mismatch_exception& ec)
                {
                    fn_reply(this, msgid, to_string(rpc_status::invalid_parameter), nullptr);
                }
                catch (archive::error::archive_exception& ec)
                {
                    throw detail::rpc_handler_fatal_state{};
                }
                catch (std::exception& ec)
                {
                    fn_reply(this, msgid, ec.what(), nullptr);
                }
            }

            _reader.end_array(ctx);  // end reading params
        }
        else
        {
            _reader >> nullptr;

            lock_guard _wr_lock_{_write_lock};
            _writer.array_push(4);
            _writer << rpc_type::reply
                    << msgid
                    << to_string(rpc_status::method_not_exist)
                    << nullptr;
            _writer.array_pop();

            _writer.flush();
        }
    }

    void _handle_notify()
    {
        //  1. read method name
        //  2. invoke handler

        int msgid = {};
        _reader >> _method_name_buf;

        if (auto pair = find_ptr(_get_services(), _method_name_buf))
        {
            auto& srv = pair->second;
            auto ctx  = _reader.begin_array();

            if (_reader.elem_left() >= srv->num_params())
            {
                try
                {
                    srv->invoke(_profile, _reader, nullptr, nullptr);
                }
                catch (remote_handler_exception&)
                {
                    ;  // On handler error, do nothing as this is notifying handler.
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
    // Event dispatcher. Default behavior is invoking in-place.
    dispatch_function _dispatch;

    // Defined services
    service_info const _service;

    // List of created sessions.
    std::set<session_ptr, std::owner_less<>> _session_sources;
    std::deque<session_wptr> _sessions;

    mutable thread::event_wait _session_notify;
    pool<std::vector<session_ptr>> _notify_pool;

    //! Context monitor
    std::weak_ptr<if_context_monitor> _monitor;

   public:
    std::chrono::milliseconds global_timeout{6'000'000};

   public:
    /**
     * Create new context, with appropriate dispatcher function.
     */
    explicit context(
            service_info service                      = {},
            dispatch_function dispatcher              = [](auto&& fn) { fn(); },
            std::weak_ptr<if_context_monitor> monitor = {})
            : _dispatch(std::move(dispatcher)),
              _service(std::move(service)),
              _monitor(std::move(monitor))
    {
    }

    context(service_info service, std::weak_ptr<if_context_monitor> monitor)
            : context(
                    std::move(service),
                    [](auto&& fn) { fn(); },
                    std::move(monitor))
    {
    }

    /**
     * Copying/Moving object address is not permitted, as sessions
     */
    context& operator=(context const&) = delete;
    context& operator=(context&&) noexcept = delete;
    context(context const&)                = delete;
    context(context&&) noexcept            = delete;

    /**
     * Call RPC function. Will be load-balanced automatically.
     *
     * @tparam RetVal_
     * @tparam Params_
     * @param retval receive output to this memory
     * @param params
     * @return
     *
     * @throw remote_reply_exception
     */
    template <typename RetVal_, typename... Params_>
    auto rpc(RetVal_ retval, std::string_view method, Params_&&... params) -> rpc_status
    {
        auto session = _checkout();
        if (not session) { return rpc_status::timeout; }

        try
        {
            auto request = session->rpc_send(retval, method, std::forward<Params_>(params)...);
            auto result  = session->rpc_wait(request);

            _checkin(std::move(session));
            return result;
        }
        catch (remote_reply_exception& ec)
        {
            _checkin(std::move(session));
            throw ec;
        }
        catch (invalid_connection&)
        {
            // do nothing, session will be disposed automatically by disposing pointer.
            return rpc_status::dead_peer;
        }
        catch (archive::error::archive_exception&)
        {
            // same as above.
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
        catch (archive::error::archive_exception&)
        {
            // same as above.
        }
    }

    /**
     * Notify all sessions
     */
    template <typename... Params_>
    void notify_all(std::string_view method, Params_&&... params)
    {
        auto all = _notify_pool.checkout();
        all->clear();

        _session_notify.critical_section(
                [&] {
                    all->reserve(_sessions.size());

                    for (auto& wp : _sessions)
                        if (auto sp = _impl_checkout(wp)) { all->emplace_back(std::move(sp)); }
                });

        for (auto& sp : *all)
        {
            try
            {
                sp->notify(method, std::forward<Params_>(params)...);
                _checkin(std::move(sp));
            }
            catch (invalid_connection&)
            {
                ;  // do nothing, let it be disposed
            }
            catch (archive::error::archive_exception&)
            {
                ;  // same as above.
            }
        }
    }

   public:
    struct session_handle
    {
        friend class context;

       private:
        session_wptr _ref;

       public:
        operator bool() const noexcept { return not _ref.expired(); }
    };

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
    session_handle create_session(session_config const& conf, Args_&&... args)
    {
        // put created session reference to sessions_source and sessions both.
        auto connection = std::make_unique<Conn_>(std::forward<Args_>(args)...);
        auto session    = std::make_shared<detail::session>(this, conf, std::move(connection), _monitor);

        session_handle handle;
        handle._ref = session;

        // Initialize explicitly
        session->_start_();

        // notify session creation, for rpc requests which is waiting for
        //  any valid session.
        _session_notify.notify_all(
                [&] {
                    _sessions.push_back(session);
                    _session_sources.emplace(std::move(session));
                });

        return handle;
    }

    /**
     * Remove session with given handle.
     */
    bool erase_session(session_handle handle)
    {
        return _erase_session(std::move(handle._ref));
    }

    /**
     * Get number of active sessions
     */
    size_t session_count() const
    {
        size_t n;
        _session_notify.critical_section([&] { n = _sessions.size(); });
        return n;
    }

    void disconnect_all()
    {
        _session_notify.critical_section(
                [&] {
                    for (auto& wp : _sessions)
                        _erase_session(wp);

                    _sessions.clear();
                });
    }

   protected:
    void dispatch(function<void()> message) { _dispatch(std::move(message)); }

   private:
    session_ptr _checkout(bool wait = true)
    {
        using namespace std::chrono_literals;

        session_ptr session = {};
        auto predicate =
                [&] {
                    int max_cycle = _sessions.size();
                    for (; not _sessions.empty() && max_cycle > 0; --max_cycle)
                    {
                        // Everytime _checkout() is called, different session will be selected.
                        auto ptr = std::move(_sessions.front());
                        _sessions.pop_front();

                        if ((session = _impl_checkout(ptr)) == nullptr)
                            continue;  // dispose wptr

                        // push weak pointer back for load balancing between active sessions
                        _sessions.push_back(ptr);

                        // Only 2 concurrent request can be accepted per session.
                        if (session && session->_refcnt > 2)
                        {
                            --session->_refcnt;
                            session = nullptr;

                            continue;
                        }

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

            assert(session->_refcnt == 0);
        }
        else if ((session = ptr.lock()) == nullptr)
        {
            // given session seems expired ...
            // do nothing, as given pointer already removed from sessions ...
            ;
        }

        if (session) { ++session->_refcnt; }
        return session;
    }

    void _checkin(session_ptr&& ptr)
    {
        _session_notify.notify_one(
                [&] {
                    // If there's any other writing instance who checked out this session,
                    //  just mandate it the responsibility to return source back.
                    if (--ptr->_refcnt > 0) { return; }
                    assert(0 == ptr->_refcnt);

                    // if pointer is pending kill, do not return this to sources buffer.
                    // which will dispose given session, which was given by move operation.
                    if (ptr->pending_kill()) { return; }

                    // otherwise, return it to source buffer to keep object valid
                    _session_sources.insert(std::move(ptr));
                });
    }

    bool _erase_session(session_wptr wptr)
    {
        if (auto ptr = wptr.lock())
        {
            ptr->_conn->disconnect();
            ptr->_pending_kill.store(true, std::memory_order_release);
        }
        else
        {
            return false;  // handle already disposed.
        }

        _session_notify.critical_section(
                [this, wp = wptr] {
                    auto iter = _session_sources.find(wp);
                    if (_session_sources.end() == iter) { return; }

                    _session_sources.erase(iter);
                });

        return true;
    }
};

}  // namespace CPPHEADERS_NS_::msgpack::rpc

//
// __CPP__
//
namespace CPPHEADERS_NS_::msgpack::rpc {

inline void if_connection::notify_receive()
{
    if (auto owner = _owner.lock())
        owner->wakeup();
}

inline void if_connection::notify_disconnect()
{
    if (auto owner = _owner.lock())
        owner->dispose_self();
}

namespace detail {
inline void session::wakeup()
{
    if (_pending_kill.load(std::memory_order_acquire)) { return; }

    // do nothing if not waiting.
    if (not _waiting.exchange(false))
        assert(false && "Notification received when it's not waiting for new data ..");
    else
        _owner->dispatch(bind_front_weak(weak_from_this(), &session::_wakeup_func, this));
}

inline service_info::handler_table_type const& session::_get_services() const
{
    return _owner->_service._services_();
}

inline void session::_erase_self()
{
    _owner->_erase_session(weak_from_this());
}

}  // namespace detail
}  // namespace CPPHEADERS_NS_::msgpack::rpc