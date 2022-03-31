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

#include "../../../thread/event_wait.hxx"
#include "../../__namespace__"
#include "connection.hxx"
#include "interface.hxx"
#include "protocol_stream.hxx"
#include "remote_procedure_message_proxy.hxx"
#include "service.hxx"
#include "session_profile.hxx"

namespace CPPHEADERS_NS_::rpc {
using session_ptr = shared_ptr<class session>;

class session : public if_session, public std::enable_shared_from_this<session>
{
    // Reference to associated event procedure
    shared_ptr<if_event_proc> _event_proc;

    // Session monitor
    shared_ptr<if_session_monitor> _monitor;

    // Registered user data
    shared_ptr<void> _user_data;

    // Owning connection
    unique_ptr<if_connection_streambuf> _conn;

    // Current protocol
    unique_ptr<if_protocol_stream> _protocol;

    // Service description
    service _svc;

    // I/O Lock
    std::mutex _mtx_io;

    // Check if this session is alive.
    session_profile _profile;

    // RPC lock
    thread::event_wait _evt_rpc;

    // Current connection state for quick check.
    std::atomic_bool _valid;

    // Unique request id generator per session
    int _request_id_gen = 0;

#ifndef NDEBUG
    // Waiting validator
    std::atomic_bool _waiting = false;
#endif

   private:
    session() noexcept = default;

   public:
    /**
     * Request RPC
     */

    /**
     * Send notify
     */

    /**
     * Close
     */

    /**
     * Wait RPC for given time
     */

    /**
     * Wait RPC forever
     */

    /**
     * Abort RPC
     */

    /**
     * Get total number of received bytes
     */

   private:
    void on_data_wait_complete() noexcept override
    {
        assert(_waiting.exchange(false) == true);

        remote_procedure_message_proxy proxy;
        proxy._procedure = &*_event_proc;
        proxy._svc = &_svc;

        {
            lock_guard _lc_{_mtx_io};
            auto       state = _protocol->handle_single_message(proxy);

            switch (state) {
                default:
                    _monitor->on_receive_warning(&_profile, state);
                    [[fallthrough]];

                case protocol_stream_state::okay:
                    break;

                case protocol_stream_state::expired:
                    _valid.store(false, std::memory_order_release);
                    _monitor->on_session_expired(&_profile);

                    // Do not wait for next receive ...
                    return;
            }
        }

        assert(_waiting.exchange(true) == false);
        _conn->async_wait_data();
    }

   public:
    template <int SlotValue = 0>
    class builder
    {
        enum slot_flags {
            slot_event_proc,
            slot_connection,
            slot_protocol,

            opt_slot_user_data,
            opt_slot_monitor,
            opt_slot_service,
        };

       private:
        shared_ptr<session> _session;

       private:
        template <slot_flags... Values,
                  typename = std::enable_if_t<not(SlotValue & ((1 << Values) | ...))>>
        auto& _make_ref() noexcept
        {
            return (builder<((SlotValue | (1 << Values)) | ...)>&)*this;
        }

       public:
        auto& user_data(shared_ptr<void> ptr)
        {
            return _make_ref<opt_slot_user_data>();
        }

        // TODO

        auto build()
        {
            static_assert(SlotValue & (1 << slot_event_proc));
            static_assert(SlotValue & (1 << slot_connection));
            static_assert(SlotValue & (1 << slot_protocol));
            return std::move(_session);
        }
    };
};
}  // namespace CPPHEADERS_NS_::rpc