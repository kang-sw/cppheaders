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

#include <memory>

#include "../../detail/object_core.hxx"
#include "defs.hxx"
#include "interface.hxx"
#include "remote_procedure_message_proxy.hxx"
#include "request_handle.hxx"

namespace cpph::rpc {
/**
 * All of methods in this class will be called in critical section.
 */
class if_protocol_procedure
{
   public:
    virtual ~if_protocol_procedure() = default;

   public:
    /**
     * Initialize internal stream handler(reader/writer) with given streambuf
     */
    virtual void initialize(std::streambuf* streambuf) = 0;

    /**
     * Handle single message. This will be invoked when owning session
     *  was notified for data arrival.
     *
     * -> If received message was request/notify
     * 1.1. Retrieve method name from message
     * 1.2. Retrieve message id from message
     * 2. Find service handler from proxy.
     * 3. Fill parameter contents with internal if_writer, if_reader
     * 4. Submit packed_message_handler
     *
     * -> If received message was reply
     * 1. Retrieve message id from message
     * 2.
     */
    virtual protocol_stream_state handle_single_message(remote_procedure_message_proxy& proxy) noexcept = 0;

    /**
     * Send RPC with given message id and parameters.
     *
     * If RPC protocol's internal message identifier type is not compatible with msgid,
     *  it can internally marshal msgid into their own identifier types, and register
     *  it to table.
     *
     * If internal RPC protocol is compatible with simple integer, this function should only
     *  perform simple transfer operation.
     */
    virtual bool send_request(std::string_view method, int msgid, array_view<refl::object_const_view_t> params) noexcept = 0;

    /**
     * Send notify with given parameters.
     */
    virtual bool send_notify(std::string_view method, array_view<refl::object_const_view_t> params) noexcept = 0;

    /**
     * Send reply with given msgid and return value.
     */
    virtual bool send_reply_result(int msgid, refl::object_const_view_t retval) noexcept = 0;

    /**
     * Send reply as error
     */
    virtual bool send_reply_error(int msgid, refl::object_const_view_t error) noexcept = 0;

    /**
     * Send reply as string
     */
    virtual bool send_reply_error(int msgid, std::string_view content) noexcept = 0;

    /**
     * This is called on RPC abort sequence. This function is required
     *  when send_rpc_request function created internal message identifier mapping.
     *
     * In other cases, such as successful reply handling, releasing key mapping
     *  should be handled internally by protocol handler itself.
     */
    virtual protocol_stream_state release_key_mapping_on_abort(int msgid) noexcept { return protocol_stream_state::okay; }
};
}  // namespace cpph::rpc
