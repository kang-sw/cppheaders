// MIT License
//
// Copyright (c) 2021-2022. Seungwoo Kang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// project home: https://github.com/perfkitpp

#pragma once
#include <mutex>

#include "../../../container/flat_map.hxx"
#include "../../../memory/pool.hxx"
#include "../../../thread/event_wait.hxx"
#include "connection.hxx"
#include "interface.hxx"
#include "protocol_procedure.hxx"
#include "remote_procedure_message_proxy.hxx"
#include "service.hxx"
#include "session_profile.hxx"

namespace cpph::rpc {
using request_complete_handler = ufunction<void(error_code const&, string_view json_error)>;

template <int>
class basic_session_builder;

namespace _detail {
class empty_session_monitor : public if_session_monitor
{
   public:
    void on_session_expired(session_profile_view view) noexcept override {}
    void on_session_created(session_profile_view view) noexcept override {}
    void on_receive_warning(session_profile_view view, protocol_stream_state state) noexcept override {}
    void on_handler_error(session_profile_view view, std::exception& e) noexcept override {}

   public:
    static auto get() noexcept
    {
        auto _value = std::make_shared<empty_session_monitor>();
        return _value;
    }
};
}  // namespace _detail

class session : public if_session, public std::enable_shared_from_this<session>
{
    template <int>
    friend class basic_session_builder;

   public:
    using builder = basic_session_builder<0>;

   private:
    // Reference to associated event procedure
    shared_ptr<if_event_proc> _event_proc;

    // Session monitor
    shared_ptr<if_session_monitor> _monitor;

    // Owning connection
    unique_ptr<if_connection> _conn;

    // Current protocol
    unique_ptr<if_protocol_procedure> _protocol;

    // Service description
    service _service = service::empty_service();

    // I/O Lock. Only protects stream access
    std::mutex _mtx_protocol;

    // Cached RPC profile
    session_profile _profile;

    // Current connection state for quick check.
    std::atomic_bool _valid;

    // Autoflush enabled ?
    std::atomic_bool _manual_flush = false;

    // Connection can be closed without lock.
    std::once_flag _flag_conn_close = {};

#ifndef NDEBUG
    // Waiting validator
    std::atomic_bool _waiting = false;
#endif

   private:
    struct rpc_request_node {
        request_complete_handler handler;
        refl::object_view_t return_buffer;
        string error_buffer;
    };

    struct rpc_context {
        thread::event_wait lock;

        // Unique request id generator.
        // Generated ID value range is (0, INT_MAX]
        int idgen = 0;

        // Prevents repeated heap usage for every RPC request
        pool<rpc_request_node> request_node_pool;

        // List of active RPC requests
        flat_map<int, pool_ptr<rpc_request_node>> requests;
    };

   private:
    // Optional RPC context.
    unique_ptr<rpc_context> _rq;

   private:
    // Hides constructor from public
    enum class _ctor_hide_type {};

   public:
    /**
     * Below code prohibits external session construction
     */
    session() = delete;
    explicit session(_ctor_hide_type) noexcept {}

    /**
     * Dispose session ...
     */
    ~session() override
    {
        // Quickly close connection without acquiring lock
        std::call_once(_flag_conn_close, [this] { _conn->close(); });

        // Try close session first.
        if (lock_guard{_mtx_protocol}, _set_expired(false)) {
            // NOTE: shared_from_this() is not permitted to be called from destructor.
            _monitor->on_session_expired(&_profile);
        }

        // Uninstall request features.
        // This automatically waits for pool_ptr instance disposal.
        _rq.reset();

        // Expire protocol
        _protocol.reset();

        // Expire connection
        _conn.reset();
    }

   private:
    template <typename... Params>
    auto _create_parameter_descriptor_array(
            Params const&... params) noexcept
            -> std::array<refl::object_const_view_t, sizeof...(Params)>
    {
        constexpr auto nparam = sizeof...(Params);
        auto views = std::array<refl::object_const_view_t, nparam>{};

        // Fill parameter lists
        {
            size_t index = 0;
            ((views[index++] = refl::object_const_view_t{params}), ...);
            (void)index;
        }

        return views;
    }

   public:
    /**
     * Send request
     *
     * Return buffer must be alive
     */
    template <typename RetPtr, typename... Params>
    request_handle async_request(
            string_view method,
            request_complete_handler&& handler,
            RetPtr return_buffer,
            Params const&... params) noexcept
    {
        assert(this->is_request_enabled());
        request_handle handle = {};

        auto request = _rq->request_node_pool.checkout();
        request->handler = std::move(handler);

        bool constexpr no_return
                = (is_void_v<remove_pointer_t<RetPtr>>)
               || (is_same_v<RetPtr, nullptr_t>);

        if constexpr (no_return)
            request->return_buffer = {};
        else
            request->return_buffer = refl::object_view_t{*return_buffer};

        _rq->lock.critical_section([&] {
            handle._wp = weak_from_this();
            handle._msgid = ++_rq->idgen;

            if (_rq->idgen == std::numeric_limits<int>::max())
                _rq->idgen = 0;

            _rq->requests.try_emplace(handle._msgid, std::move(request));
        });

        auto views = _create_parameter_descriptor_array(params...);

        {
            lock_guard _{_mtx_protocol};
            auto expire = not expired() && not _protocol->send_request(method, handle._msgid, views);

            if (expire)
                _set_expired();

            if (expired()) {
                // As RPC invocation couldn't be succeeded, abort rpc and throw error to explicitly
                //  notify caller that this session is in invalid state, therefore given handler
                //  won't be invoked.
                _rq->lock.critical_section([&] { _rq->requests.erase(handle._msgid); });
                handle = {};
            } else {
                if (not relaxed(_manual_flush))
                    _protocol->flush();

                _update_rw_count();
            }
        }

        return handle;
    }

    /**
     * Send notify
     *
     * @return false when connection is expired.
     */
    template <typename... Params>
    bool notify(string_view method, Params const&... params) noexcept
    {
        auto views = _create_parameter_descriptor_array(params...);

        {
            lock_guard _lc_{_mtx_protocol};
            if (expired()) { return false; }  // perform quick check

            if (not _protocol->send_notify(method, views)) {
                _set_expired();
                return false;
            } else {
                if (not relaxed(_manual_flush))
                    _protocol->flush();

                _update_rw_count();
                return true;
            }
        }
    }

    /**
     * Flush sent data
     */
    void flush() noexcept
    {
        {
            lock_guard _lc_{_mtx_protocol};
            _protocol->flush();
        }
    }

    /**
     * Set autoflush mode
     */
    void autoflush(bool enabled) noexcept
    {
        relaxed(_manual_flush, not enabled);
    }

    /**
     * Close
     *
     * @return false if connection is already expired
     */
    bool close() noexcept
    {
        // Quickly close connection without acquiring lock.
        std::call_once(_flag_conn_close, [this] { _conn->close(); });

        lock_guard _{_mtx_protocol};
        if (expired()) { return false; }

        _set_expired();
        return true;
    }

    /**
     * Check if RPC request is enabled
     */
    bool is_request_enabled() const noexcept
    {
        return _rq != nullptr;
    }

    /**
     * Wait RPC
     */
    void wait(request_handle const& h) noexcept
    {
        _rq->lock.wait([&] {
            return not contains(_rq->requests, h._msgid);
        });
    }

    template <class Duration>
    bool wait_for(request_handle const& h, Duration&& timeout) noexcept
    {
        return _rq->lock.wait_for(std::forward<Duration>(timeout), [&] {
            return not contains(_rq->requests, h._msgid);
        });
    }

    template <class Tp>
    bool wait_until(request_handle const& h, Tp&& timeout) noexcept
    {
        return _rq->lock.wait_until(std::forward<Tp>(timeout), [&] {
            return not contains(_rq->requests, h._msgid);
        });
    }

    /**
     * Abort sent request
     */
    bool abort_request(request_handle const& h) noexcept
    {
        assert(this->is_request_enabled());
        int msgid = h._msgid;
        bool valid_abortion = false;

        // Find corresponding request, and mark aborted.
        request_complete_handler callable;

        _rq->lock.critical_section([&] {
            auto iter = _rq->requests.find(msgid);
            if (iter == _rq->requests.end()) { return; }  // Expired request ...
            if (not iter->second.valid()) { return; }     // Already under invocation ...

            // Handler invocation will occur out of critical section
            valid_abortion = true;
            callable = std::exchange(iter->second->handler, {});

            _rq->requests.erase(iter);
        });

        // msgid should be released
        if (valid_abortion) {
            auto errc = make_request_error(request_result::aborted);
            callable(errc, "\"ABORTED\"");
            _rq->lock.notify_all();

            if (lock_guard _lc_{_mtx_protocol}; not expired())
                _protocol->release_key_mapping_on_abort(msgid);
        }

        return valid_abortion;
    }

    /**
     * Get total number of received bytes
     */
    void totals(size_t* nread, size_t* nwrite) noexcept
    {
        *nread = _profile.total_read;
        *nwrite = _profile.total_write;
    }

    /**
     * Check session condition
     */
    bool expired() const noexcept
    {
        return not _valid.load(std::memory_order_acquire);
    }

    /**
     * Get profile of self
     */
    session_profile_view profile() const noexcept
    {
        return &_profile;
    }

   private:
    void on_data_wait_complete() noexcept override
    {
        assert(_waiting.exchange(false) == true);

        auto fn = bind_weak(weak_from_this(), [this] { _impl_on_data_wait_complete(); });
        _event_proc->post_internal_message(std::move(fn));
    }

    void _impl_on_data_wait_complete() noexcept
    {
        remote_procedure_message_proxy proxy = {};
        proxy._owner = this;
        proxy._svc = &_service;

        protocol_stream_state state;
        {
            lock_guard _lc_{_mtx_protocol};
            if (expired()) { return; }  // If connection is already expired, do nothing.

            state = _protocol->handle_single_message(proxy);
            _update_rw_count();
        }

        switch (state) {
            default:
                _monitor->on_receive_warning(&_profile, state);
                break;

            case protocol_stream_state::okay:
                _handle_receive_result(std::move(proxy));
                break;

            case protocol_stream_state::expired:
                _set_expired();

                // Do not run receive cycle anymore ...
                return;
        }

        assert(_waiting.exchange(true) == false);
        _conn->start_data_receive();
    }

    void request_node_lock_begin() override
    {
        _rq->lock.mutex().lock();
    }

    void request_node_lock_end() override
    {
        _rq->lock.mutex().unlock();
    }

    auto find_reply_result_buffer(int msgid) -> refl::object_view_t* override
    {
        assert(_rq->lock.mutex().try_lock() == false);

        // find corresponding reply result buffer.
        auto iter = _rq->requests.find(msgid);

        if (iter != _rq->requests.end())
            return &iter->second->return_buffer;
        else
            return nullptr;
    }

    auto find_reply_error_buffer(int msgid) -> std::string* override
    {
        assert(_rq->lock.mutex().try_lock() == false);

        auto iter = _rq->requests.find(msgid);

        if (iter != _rq->requests.end()) {
            auto bufptr = &iter->second->error_buffer;

            // Buffer cleanup only occurs right before actual usage.
            //  (Buffer may not clean as it is checked out from object pool)
            bufptr->clear();
            return bufptr;
        } else
            return {};
    }

   private:
    void _initialize()
    {
        // Unique ID generator for local scope
        // TODO: is UUID based world-wide id required?
        static std::atomic_size_t _idgen = 0;

        // Fill basic profile info
        _profile.w_self = weak_from_this();
        _profile.local_id = ++_idgen;
        _profile.peer_name = _conn->peer_name;

        // Validate connection
        _conn->_wowner = weak_from_this();

        // Initialize protocol with given client
        _protocol->initialize(_conn->streambuf());

        // Session is currently valid
        _valid.store(true);

        // Notify client that monitoring has begun
        if (not _monitor) { _monitor = _detail::empty_session_monitor::get(); }
        _monitor->on_session_created(&_profile);

        // Start initial receive
#ifndef NDEBUG
        _waiting.store(true);
#endif
        _conn->start_data_receive();
    }

    bool _set_expired(bool call_monitor = true)
    {
        std::call_once(_flag_conn_close, [this] { _conn->close(); });

        if (_valid.exchange(false)) {
            do {
                if (not is_request_enabled()) { break; }

                // In RPC critical section ...
                lock_guard _{_rq->lock.mutex()};

                // Iterate all requests, and set them aborted
                for (auto& [key, req] : _rq->requests) {
                    auto fn_abort_request =
                            [request = move(req)] {
                                auto errc = make_request_error(request_result::aborted);
                                request->handler(errc, {});
                            };

                    // These should be handled inside
                    _event_proc->post_rpc_completion(std::move(fn_abort_request));
                    _protocol->release_key_mapping_on_abort(key);
                }

                _rq->requests.clear();
            } while (false);

            // NOTE: Added call_monitor selection flag, since calling shared_from_this() from
            //   destructor causes crash.
            if (call_monitor) {
                // NOTE: As _set_expired() method is only called when _mtx_protocol is occupied,
                //  calling monitor callback inside of this can easily cause deadlock. Thus invoking
                //  monitor callback was moved to outside of function scope.
                _event_proc->post_internal_message(
                        [this, self = shared_from_this()] {
                            _monitor->on_session_expired(&_profile);
                        });
            }

            return true;
        } else {
            return false;
        }
    }

    void _update_rw_count()
    {
        _conn->get_total_rw(&_profile.total_read, &_profile.total_write);
    }

    void _handle_reply(int msgid, bool successful)
    {
        decltype(_rq->requests)::mapped_type request;

        _rq->lock.critical_section([&] {
            auto iter = _rq->requests.find(msgid);
            if (iter == _rq->requests.end()) { return; }

            request = move(iter->second);
        });

        if (not request.valid())
            return;  // Request seems already expired, do nothing.

        error_code errc = {};
        string_view errmsg = {};

        if (not successful) {
            errc = make_request_error(request_result::exception_returned);
            errmsg = request->error_buffer;
        }

        // Invoke RPC request handler
        request->handler(errc, errmsg);
        request = {};  // Checkin allocated request node

        // Delete request node after handler invocation, to asure 'wait' returns after
        _rq->lock.notify_all([&] { _rq->requests.erase(msgid); });
    }

    void _handle_receive_result(remote_procedure_message_proxy&& proxy)
    {
        using proxy_flag = remote_procedure_message_proxy::proxy_type;

        switch (proxy._type) {
            case proxy_flag::request:
                assert(proxy._handler);
                {
                    auto fn_handle_rpc =
                            [this,
                             msgid = proxy._rpc_msgid,
                             handler = move(*proxy._handler)]() mutable {
                                try {
                                    auto rval = move(handler).invoke(_profile);

                                    if (lock_guard _lc_{_mtx_protocol}; not expired())
                                        _protocol->send_reply_result(msgid, rval.view());
                                } catch (service_handler_exception& e) {
                                    // Send reply with error in object
                                    _monitor->on_handler_error(&_profile, e);

                                    if (lock_guard _lc_{_mtx_protocol}; not expired())
                                        _protocol->send_reply_error(msgid, e.data().view());
                                } catch (std::exception& e) {
                                    // Send reply with error in string
                                    _monitor->on_handler_error(&_profile, e);

                                    if (lock_guard _lc_{_mtx_protocol}; not expired())
                                        _protocol->send_reply_error(msgid, e.what());
                                }
                            };

                    _event_proc->post_handler_callback(
                            bind_front_weak(weak_from_this(), move(fn_handle_rpc)));
                }
                break;

            case proxy_flag::notify:
                assert(proxy._handler);
                {
                    auto fn_handle_notify =
                            [this, handler = move(*proxy._handler)]() mutable {
                                try {
                                    // Discard return value, as this is notification call
                                    move(handler).invoke(_profile);
                                } catch (std::exception& e) {
                                    // Simply report error to monitor
                                    _monitor->on_handler_error(&_profile, e);
                                }
                            };

                    _event_proc->post_handler_callback(
                            bind_front_weak(weak_from_this(), move(fn_handle_notify)));
                }
                break;

            case proxy_flag::reply_okay:
                assert(proxy._rpc_msgid);

                // Notify request result is ready.
                //  * Assumes reply value is already copied during protocol handler invocation
                _event_proc->post_rpc_completion(
                        bind_front_weak(
                                weak_from_this(),
                                &session::_handle_reply, this, proxy._rpc_msgid, true));
                break;

            case proxy_flag::reply_error:
                assert(proxy._rpc_msgid);

                // Do same as above.
                _event_proc->post_rpc_completion(
                        bind_front_weak(
                                weak_from_this(),
                                &session::_handle_reply, this, proxy._rpc_msgid, false));
                break;

            case remote_procedure_message_proxy::proxy_type::reply_expired:
                // Do nothing.
                break;

            default:
            case proxy_flag::in_progress:
            case proxy_flag::none:
                assert(false && "A logical bug inside of proxy handler");
                break;
        }
    }
};
}  // namespace cpph::rpc