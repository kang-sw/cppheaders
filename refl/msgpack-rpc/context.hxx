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

#include "../../container/sorted_vector.hxx"
#include "../../functional.hxx"
#include "../../memory/pool.hxx"
#include "../../thread/event_wait.hxx"
#include "../../thread/locked.hxx"
#include "../../thread/thread_pool.hxx"
#include "../../timer.hxx"
#include "../../utility/singleton.hxx"
#include "../__namespace__"
#include "../archive/msgpack-reader.hxx"
#include "../archive/msgpack-writer.hxx"
#include "../detail/object_core.hxx"
#include "../detail/primitives.hxx"
#include "defs.hxx"
#include "errors.hxx"
#include "request_handle.hxx"
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
    std::string                    _peer;

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
     * Returns total number of written/read bytes.
     */
    virtual void totals(size_t* nwrite, size_t* nread) const = 0;

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
     * Start communication. Before this call, call to notify_one() cause crash.
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
        bool                      use_integer_key = true;
        std::chrono::microseconds timeout = {};
    };

   private:
    struct request_info
    {
        using handle_type = std::map<int, request_info>::iterator;

        function<void(reader*, std::exception_ptr)> completion_handler;
    };

   private:
    friend class rpc::context;

    std::weak_ptr<nullptr_t> _owner_fence;
    context*                 _owner = {};
    config                   _conf;

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
    spinlock     _write_lock;
    volatile int _msgid_gen = 0;
    std::string  _method_name_buf;

    // Check function is waiting for awake.
    std::atomic_bool _waiting{false};

    // RPC reply table. Old ones may set timeout exceptions
    std::map<int, request_info> _requests;
    std::vector<int>            _waiting_ids;

    // Recv event wait
    thread::event_wait _rpc_notify;

    // Check if pending kill
    std::atomic_bool _pending_kill = false;

    // write refcount
    volatile int                      _refcnt = 0;

    std::weak_ptr<if_context_monitor> _monitor;

   public:
    using request_handle_type = decltype(_requests)::iterator;

   public:
    session(context*                          owner,
            config const&                     conf,
            std::unique_ptr<if_connection>    conn,
            std::weak_ptr<if_context_monitor> wmonitor) noexcept
            : _owner(owner), _conf(conf), _conn(std::move(conn)), _monitor(std::move(wmonitor))
    {
        if (_conf.timeout.count() == 0) { _conf.timeout = std::chrono::hours{2400}; }

        // Fill profile info
        _profile.peer_name = _conn->peer();
        _conn->set_timeout(_conf.timeout);
    }

    ~session() noexcept
    {
        if (auto monitor = _monitor.lock()) { monitor->on_dispose_session(_profile); }
    }

   public:
    template <typename RetPtr_, typename... Params_,
              typename CompletionToken_,
              typename = std::enable_if_t<std::is_invocable_v<CompletionToken_, std::exception const*>>>
    auto async_rpc(RetPtr_ result, std::string_view method, CompletionToken_&& handler, Params_&&... params)
    {
        decltype(_requests)::iterator request;
        int                           msgid;

        // create reply slot
        _rpc_notify.critical_section([&] {
            msgid = ++_msgid_gen < 0 ? (_msgid_gen = 1) : _msgid_gen;
            auto pair = _requests.try_emplace(msgid);
            request = pair.first;
            assert(pair.second);  // message id should never duplicate

            _waiting_ids.push_back(msgid);

            // This will be invoked when return value from remote is ready, and will directly
            //  deserialize the result into provided argument.
            request->second.completion_handler =
                    [this, msgid, result, handler = std::forward<CompletionToken_>(handler)]  //
                    (reader * rd, std::exception_ptr except) mutable {
                        auto fn_invoke_handler =
                                [this, msgid, handler = move(handler)]  //
                                (const std::exception_ptr& actual_except) mutable {
                                    try {
                                        if (actual_except)
                                            rethrow_exception(actual_except);

                                        handler((std::exception*)nullptr);
                                    } catch (std::exception& ec) {
                                        handler(&ec);
                                    }

                                    this->_rpc_notify.notify_all([&] {
                                        auto iter = find(_waiting_ids, msgid);
                                        *iter = _waiting_ids.back();
                                        _waiting_ids.pop_back();
                                    });
                                };

                        auto do_post_with =
                                [this, &fn_invoke_handler]  //
                                (std::exception_ptr except) {
                                    _post(bind_front(std::move(fn_invoke_handler), std::move(except)));
                                };

                        if (except) {
                            do_post_with(std::move(except));
                        } else {
                            try {
                                if constexpr (std::is_null_pointer_v<RetPtr_>) {
                                    *rd >> nullptr;
                                } else if constexpr (std::is_void_v<std::remove_pointer_t<RetPtr_>>) {
                                    *rd >> nullptr;
                                } else {
                                    if (result == nullptr)
                                        *rd >> nullptr;  // skip
                                    else
                                        *rd >> *result;
                                }

                                do_post_with({});
                            } catch (type_mismatch_exception& e) {
                                rpc_error exception{rpc_status::invalid_return_type};
                                do_post_with(std::make_exception_ptr(exception));
                            } catch (std::exception&) {
                                do_post_with(std::current_exception());
                                throw detail::rpc_handler_fatal_state{};
                            }
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

        return msgid;
    }

    template <typename Duration_>
    bool wait_rpc(int msgid, Duration_&& duration)
    {
        auto predicate = [&] { return _waiting_ids.end() == find(_waiting_ids, msgid); };
        return _rpc_notify.wait_for(duration, predicate);
    }

    bool abort_rpc(int msgid)
    {
        bool                                       found = false;
        decltype(request_info::completion_handler) handler;
        _rpc_notify.critical_section([&] {
            if (auto iter = _requests.find(msgid); iter != _requests.end()) {
                found = true;
                handler = std::move(iter->second.completion_handler);
                _requests.erase(iter);
            }
        });

        if (found) {
            rpc_error error{rpc_status::aborted};
            handler(nullptr, std::make_exception_ptr(error));
        }

        return found;
    }

    template <typename... Params_>
    void notify_one(std::string_view method, Params_&&... params)
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

    void cancel_all_requests()
    {
        std::vector<int> reqs;

        _rpc_notify.notify_all(
                [&] {
                    reqs.reserve(_requests.size());
                    rpc_error error{rpc_status::aborted};
                    for (auto& [msgid, elem] : _requests) { reqs.push_back(msgid); }
                });

        for (auto msgid : reqs) { abort_rpc(msgid); }
    }

    void dispose_self()
    {
        _erase_self();
    }

    auto lock_write() { return std::unique_lock{_write_lock}; }
    auto try_lock_write() { return std::unique_lock{_write_lock, std::try_to_lock}; }

    bool pending_kill() const noexcept { return _pending_kill.load(std::memory_order_acquire); }

    void _start_();

   private:
    void _post(function<void()>&& fn);

    void _wakeup_func()
    {
        // NOTE: This function guaranteed to not be re-entered multiple times on single session
        try {
            // Assume data is ready to be read.
            auto key = _reader.begin_array();
            _conn->totals(&_profile.total_write, &_profile.total_read);

            rpc_type type;

            switch (_reader >> type, type) {
                case rpc_type::request: _handle_request(); break;
                case rpc_type::notify: _handle_notify(); break;
                case rpc_type::reply: _handle_reply(); break;

                default: throw invalid_connection{};
            }

            _reader.end_array(key);

            // waiting for next input.
            _waiting.store(true, std::memory_order_release);
            _conn->begin_wait();
        } catch (detail::rpc_handler_fatal_state&) {
            _erase_self();
        } catch (invalid_connection&) {
            _erase_self();
        } catch (archive::error::archive_exception&) {
            _erase_self();
        }
    }

    void _handle_reply()
    {
        int msgid = -1;
        _reader >> msgid;

        //  1. find corresponding reply to msgid
        //  2. fill promise with value or exception
        decltype(request_info::completion_handler) handler;

        _rpc_notify.critical_section([&] {
            auto iter = _requests.find(msgid);
            if (iter == _requests.end()) { return; }

            handler = std::move(iter->second.completion_handler);
            _requests.erase(iter);
        });

        // Expired message. Simply ignore.
        if (not handler) { return; }

        if (_reader.is_null_next())  // no error
        {
            _reader >> nullptr;
            handler(&_reader, nullptr);
        } else {
            // TODO: Extract 'errmsg' as object and dump to string!
            std::string errmsg;
            _reader >> errmsg;
            _reader >> nullptr;  // skip payload

            if (auto errc = from_string(errmsg); rpc_status::unknown_error == errc) {
                remote_reply_exception exception{errmsg};
                handler(nullptr, std::make_exception_ptr(exception));
            } else {
                rpc_error exception{errc};
                handler(nullptr, std::make_exception_ptr(exception));
            }
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

        if (auto pair = find_ptr(_get_services(), _method_name_buf)) {
            auto& srv = pair->second;
            auto  ctx = _reader.begin_array();  // begin reading params

            if (_reader.elem_left() < srv->num_params()) {
                // number of parameters are insufficient.
                fn_reply(this, msgid, to_string(rpc_status::invalid_parameter), nullptr);
            } else {
                try {
                    struct uobj_t
                    {
                        session* self;
                        int      msgid;
                    } uobj{this, msgid};

                    srv->invoke(
                            _profile, _reader,
                            [](void* pvself, refl::object_const_view_t data) {
                                auto self = ((uobj_t*)pvself)->self;
                                auto msgid = ((uobj_t*)pvself)->msgid;

                                fn_reply(self, msgid, nullptr, data);
                            },
                            &uobj);
                } catch (remote_handler_exception& ec) {
                    fn_reply(this, msgid, ec.view(), nullptr);
                } catch (type_mismatch_exception& ec) {
                    fn_reply(this, msgid, to_string(rpc_status::invalid_parameter), nullptr);
                } catch (archive::error::archive_exception& ec) {
                    throw detail::rpc_handler_fatal_state{};
                } catch (std::exception& ec) {
                    fn_reply(this, msgid, ec.what(), nullptr);
                }
            }

            _reader.end_array(ctx);  // end reading params
        } else {
            _reader >> nullptr;
            fn_reply(this, msgid, to_string(rpc_status::method_not_exist), nullptr);
        }
    }

    void _handle_notify()
    {
        //  1. read method name
        //  2. invoke handler

        int msgid = {};
        _reader >> _method_name_buf;

        if (auto pair = find_ptr(_get_services(), _method_name_buf)) {
            auto& srv = pair->second;
            auto  ctx = _reader.begin_array();

            if (_reader.elem_left() >= srv->num_params()) {
                try {
                    srv->invoke(_profile, _reader, nullptr, nullptr);
                } catch (remote_handler_exception&) {
                    ;  // On handler error, do nothing as this is notifying handler.
                } catch (type_mismatch_exception&) {
                    ;
                } catch (archive::error::archive_exception&) {
                    throw detail::rpc_handler_fatal_state{};
                }
            } else {
                ;  // errnous situation ... as this is notify, just ignore.
            }

            _reader.end_array(ctx);
        } else {
            _reader >> nullptr;
        }
    }

    service_info::handler_table_type const& _get_services() const;
    void                                    _erase_self();
};
}  // namespace detail

using session_config = detail::session::config;

class context
{
    friend class detail::session;

    using session_ptr = std::shared_ptr<detail::session>;
    using session_wptr = std::weak_ptr<detail::session>;

   public:
    using post_function = function<void(function<void()>&&)>;

   private:
    // Event poster. Default behavior is invoking in-place.
    post_function _post;

    // Defined services
    service_info const _service;

    // List of created sessions.
    std::set<session_ptr, std::owner_less<>> _session_sources;
    std::deque<session_wptr>                 _sessions;

    mutable thread::event_wait               _session_notify;
    pool<std::vector<session_ptr>>           _notify_pool;

    //! Context monitor
    std::weak_ptr<if_context_monitor> _monitor;

    // Lifetime guard
    std::shared_ptr<nullptr_t> _fence = std::make_shared<nullptr_t>();

    // Offset of Rx/Tx values from disposed sessions
    size_t _offset_rx = 0;
    size_t _offset_tx = 0;

   public:
    std::chrono::milliseconds global_timeout{6'000'000};

   public:
    class wrap_thread_pool_t
    {
        thread_pool* _ptr;

       public:
        explicit wrap_thread_pool_t() noexcept : _ptr{&default_singleton<thread_pool>()} {}
        void operator()(function<void()>&& fn) { _ptr->post(std::move(fn)); }
    };

   public:
    /**
     * Create new context, with appropriate poster function.
     */
    explicit context(
            service_info                      service = {},
            post_function                     poster = wrap_thread_pool_t(),
            std::weak_ptr<if_context_monitor> monitor = {})
            : _post(std::move(poster)),
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
    context(context const&) = delete;
    context(context&&) noexcept = delete;

    ~context() noexcept
    {
        std::weak_ptr anchor{_fence};
        disconnect_all();

        _fence.reset();
        while (not anchor.expired()) { std::this_thread::yield(); }
    }

   private:
    template <typename RetPtr_,
              typename CompletionToken_,
              typename... Params_>
    auto _async_rpc(
            session_ptr&       session,
            RetPtr_            retval,
            std::string_view   method,
            CompletionToken_&& handler,
            Params_&&... params) -> async_rpc_result::type
    {
        try {
            auto handler_impl =
                    [this, session,
                     handler = std::forward<CompletionToken_>(handler)]  //
                    (auto&& e) mutable {
                        _checkin(std::move(session));
                        handler(e);
                    };

            auto msgid = session->async_rpc(
                    retval, method,
                    std::move(handler_impl),
                    std::forward<Params_>(params)...);

            return async_rpc_result::type{msgid};
        } catch (invalid_connection&) {
            // As this connection is invalidated during writing, try find another session.
            return async_rpc_result::invalid_connection;
        } catch (archive::error::archive_exception&) {
            // Archive exception is possibly raised when caller(parameter) is invalid.
            // In this case, abort execution of this function.
            _checkin(std::move(session));
            return async_rpc_result::invalid_parameters;
        }
    }

   public:
    template <typename RetPtr_,
              typename CompletionToken_,
              typename... Params_>
    auto async_rpc(RetPtr_            retval,
                   std::string_view   method,
                   CompletionToken_&& handler,
                   Params_&&... params) -> request_handle
    {
        // Basically, iterate until succeeds.
        request_handle result;

        for (;;) {
            auto session = _checkout();
            if (not session) {
                result._msgid = async_rpc_result::no_active_connection;
                break;
            }

            result._wp = session;
            auto msgid = _async_rpc(
                    session, retval, method,
                    std::forward<CompletionToken_>(handler),
                    std::forward<Params_>(params)...);

            if (msgid != async_rpc_result::invalid_connection) {
                result._msgid = msgid;
                break;
            }
        }

        return result;
    }

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
    template <typename RetPtr_, typename... Params_, class Rep_, class Ratio_>
    auto rpc(RetPtr_                             retval,
             std::string_view                    method,
             std::chrono::duration<Rep_, Ratio_> timeout,
             Params_&&... params) -> rpc_status
    {
        volatile rpc_status status = rpc_status::unknown_error;
        std::exception_ptr  user_except;

        for (;;) {
            auto session = _checkout();
            if (not session) { return rpc_status::timeout; }

            auto fn_on_complete =
                    [&](std::exception const* e) {
                        if (not e)
                            status = rpc_status::okay;
                        else if (auto p = dynamic_cast<rpc_error const*>(e))
                            status = p->error_code;
                        else if (auto p2 = dynamic_cast<remote_reply_exception const*>(e))
                            user_except = std::make_exception_ptr(*p2);
                    };

            int msgid = _async_rpc(
                    session, retval, method,
                    std::move(fn_on_complete),
                    std::forward<Params_>(params)...);

            if (msgid > 0) {
                // Wait until RPC invocation finished
                if (not session->wait_rpc(msgid, timeout))
                    return session->abort_rpc(msgid), rpc_status::timeout;

                if (*&user_except)
                    std::rethrow_exception(user_except);

                if constexpr (std::is_pointer_v<RetPtr_>)
                    if constexpr (not std::is_void_v<std::remove_pointer_t<RetPtr_>>)
                        if (retval && status == rpc_status::okay) { (void)*retval; }

                return status;
            } else if (msgid == async_rpc_result::invalid_connection) {
                continue;  // find other session
            } else {
                return rpc_status::internal_error;
            }
        }
    }

    template <typename RetPtr_, typename... Params_>
    auto rpc(RetPtr_          retval,
             std::string_view method,
             Params_&&... params) -> rpc_status
    {
        return rpc(retval, method, global_timeout, std::forward<Params_>(params)...);
    }

    /**
     * Notify single session
     */
    template <typename... Params_>
    void notify_one(std::string_view method, Params_&&... params)
    {
        try {
            using namespace std::chrono_literals;

            auto session = _checkout(false);
            if (not session) { return; }

            session->notify_one(method, std::forward<Params_>(params)...);
            _checkin(std::move(session));
        } catch (invalid_connection& e) {
            // do nothing, session will be disposed automatically by disposing pointer.
        } catch (archive::error::archive_exception&) {
            // same as above.
        }
    }

    /**
     * Notify all sessions
     */
    template <
            typename... Params_, typename Qualify_,
            typename = std::enable_if_t<std::is_invocable_r_v<bool, Qualify_, session_profile const&>>>
    size_t notify_all(std::string_view method, Qualify_&& qualifier, Params_&&... params)
    {
        size_t num_sent = 0;
        auto   all = _notify_pool.checkout();

        _session_notify.critical_section(
                [&] {
                    all->reserve(_sessions.size());

                    for (auto& wp : _sessions)
                        if (auto sp = _impl_checkout(wp)) { all->emplace_back(std::move(sp)); }
                });

        for (auto& sp : *all) {
            if (not qualifier(sp->_profile)) {
                _checkin(std::move(sp));
                continue;
            }

            try {
                sp->notify_one(method, std::forward<Params_>(params)...);
                _checkin(std::move(sp));

                ++num_sent;
            } catch (invalid_connection&) {
                ;  // do nothing, let it be disposed
            } catch (archive::error::archive_exception&) {
                ;  // same as above.
            }
        }

        all->clear();
        return num_sent;
    }

    template <typename... Params_>
    size_t notify_all(std::string_view method, Params_&&... params)
    {
        auto fn_always = [](auto&&) { return true; };
        return notify_all(method, fn_always, std::forward<Params_>(params)...);
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
        auto           connection = std::make_unique<Conn_>(std::forward<Args_>(args)...);
        auto           session = std::make_shared<detail::session>(this, conf, std::move(connection), _monitor);

        session_handle handle;
        handle._ref = session;

        // Initialize explicitly
        session->_start_();

        // notify session creation, for rpc requests which is waiting for
        //  any valid session.
        _session_notify.notify_all(
                [&] {
                    _sessions.push_back(session);
                    _session_sources.emplace(session);
                });

        // Notify creation to monitor
        if (auto monitor = _monitor.lock()) { monitor->on_new_session(session->_profile); }
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
        _session_notify.critical_section(
                [&] {
                    n = count_if(_sessions, [](auto&& wp) { return not wp.expired(); });
                });
        return n;
    }

    /**
     * Return total I/O bytes
     */
    void totals(size_t* num_read, size_t* num_write)
    {
        _session_notify.critical_section(
                [&] {
                    *num_write = _offset_tx;
                    *num_read = _offset_rx;

                    for (auto& wp : _sessions)
                        if (auto sess = wp.lock()) {
                            *num_write += sess->_profile.total_write;
                            *num_read += sess->_profile.total_read;
                        }
                });
    }

    /**
     * Disconnect all open sessions
     */
    void disconnect_all()
    {
        decltype(_sessions) clone;
        _session_notify.critical_section(
                [&] {
                    std::swap(clone, _sessions);
                });

        for (auto& wp : clone) {
            using namespace std::literals;
            _erase_session(wp);
        }
    }

   protected:
    void post(function<void()>&& message) { _post(std::move(message)); }

   private:
    session_ptr _checkout(bool wait = true)
    {
        using namespace std::chrono_literals;

        session_ptr session = {};
        auto        predicate =
                [&] {
                    int max_cycle = _sessions.size();
                    for (; not _sessions.empty() && max_cycle > 0; --max_cycle) {
                        // Everytime _checkout() is called, different session will be selected.
                        auto ptr = std::move(_sessions.front());
                        _sessions.pop_front();

                        if ((session = _impl_checkout(ptr)) == nullptr)
                            continue;  // dispose wptr

                        // push weak pointer back for load balancing between active sessions
                        _sessions.push_back(ptr);

                        // Only 2 concurrent request can be accepted per session.
                        if (max_cycle > 1 && session && session->_refcnt > 2) {
                            --session->_refcnt;
                            session = nullptr;

                            continue;
                        }

                        return true;
                    }

                    if (_sessions.empty()) { return true; }

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

        auto        source = _session_sources.find(ptr);
        if (source != _session_sources.end()) {  // if given session is never occupied by any other context ..
            session = *source;
            _session_sources.erase(source);

            assert(session->_refcnt == 0);
        } else if ((session = ptr.lock()) == nullptr) {
            // given session seems expired ...
            // do nothing, as given pointer already removed from sessions ...
            ;
        }

        if (session) {
            if (session->pending_kill())
                session = {};
            else
                ++session->_refcnt;
        }

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
        if (auto ptr = wptr.lock()) {
            if (not ptr->_pending_kill.exchange(true)) {
                ptr->cancel_all_requests();
                ptr->_conn->disconnect();

                _offset_tx += ptr->_profile.total_write;
                _offset_rx += ptr->_profile.total_read;
            }
        } else {
            return false;  // handle already disposed.
        }

        _session_notify.critical_section(
                [this, wp = wptr] {
                    if (auto iter = _session_sources.find(wp); iter != _session_sources.end())
                        _session_sources.erase(iter);

                    auto pred_sessions = [wp](auto&& ptr) { return ptr_equals(wp, ptr); };
                    if (auto iter = find_if(_sessions, pred_sessions); iter != _sessions.end())
                        _sessions.erase(iter);
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
        _wakeup_func();
    //_owner->post(bind_front_weak(weak_from_this(), &session::_wakeup_func, this));
}

inline service_info::handler_table_type const& session::_get_services() const
{
    return _owner->_service._services_();
}

inline void session::_erase_self()
{
    if (auto _lc_ = _owner_fence.lock())
        _owner->_erase_session(weak_from_this());
}

inline void session::_start_()
{
    // As weak_from_this cannot be called inside of constructor,
    //  initialization of each sessions must be called explicitly.
    _waiting = true;
    _owner_fence = _owner->_fence;
    _conn->_init_(weak_from_this());
}

inline void session::_post(function<void()>&& fn)
{
    _owner->post(std::move(fn));
}

}  // namespace detail
}  // namespace CPPHEADERS_NS_::msgpack::rpc