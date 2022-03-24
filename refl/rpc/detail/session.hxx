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

#include "../../__namespace__"
#include "connection.hxx"
#include "interface.hxx"
#include "protocol_stream.hxx"
#include "remote_procedure_message_proxy.hxx"
#include "service.hxx"

namespace CPPHEADERS_NS_::rpc {
using session_ptr = shared_ptr<class session>;

class session : public if_session, public std::enable_shared_from_this<session>
{
    // Reference to associated event procedure
    shared_ptr<if_event_proc> _event_proc;

    // Owning connection
    unique_ptr<if_connection_streambuf> _conn;

    // Current protocol
    unique_ptr<if_protocol_stream> _protocol;

    // Service description
    service _svc;

    // I/O Lock
    std::mutex _mtx_io;

    // Check if this session is alive.

   public:
   private:
    void on_data_wait_complete() override
    {
        remote_procedure_message_proxy proxy;
        proxy._procedure = &*_event_proc;
        proxy._svc = &_svc;

        {
            lock_guard _lc_{_mtx_io};
            auto       state = _protocol->handle_single_message(proxy);

            if (state == protocol_stream_state::irreversible) {
                // TODO: notify monitor that this session is dead!
            }
        }
    }
};

}  // namespace CPPHEADERS_NS_::rpc