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
#include "../../__namespace__"
#include "../../archive/msgpack-reader.hxx"
#include "../../archive/msgpack-writer.hxx"
#include "../../detail/primitives.hxx"
#include "../detail/protocol_stream.hxx"

namespace CPPHEADERS_NS_::rpc::protocol {

class msgpack : public if_protocol_stream
{
    enum class msgtype {
        request = 0,
        reply = 1,
        notify = 2
    };

   private:
    archive::msgpack::writer _write{nullptr, 8};
    archive::msgpack::reader _read{nullptr, 8};

   public:
    void initialize(if_connection_streambuf* streambuf) override
    {
        _write.rdbuf(streambuf);
        _read.rdbuf(streambuf);
    }

    protocol_stream_state handle_single_message(remote_procedure_message_proxy& proxy) noexcept override
    {
        return protocol_stream_state::expired;
    }

    bool send_request(std::string_view method, int msgid, array_view<refl::object_view_t> params) noexcept override
    {
        try {
            _write.array_push(4);
            _write << msgtype::request;
            _write << msgid;
            _write << method;

            _write.array_push(params.size());
            for (auto& p : params) { _write << p; }
            _write.array_pop();

            _write.array_pop();
            _write.flush();

            return true;
        } catch (std::exception& e) {
            return false;
        }
    }

    bool send_notify(std::string_view method, array_view<refl::object_view_t> params) noexcept override
    {
        try {
            _write.array_push(3);
            _write << msgtype::notify;
            _write << method;

            _write.array_push(params.size());
            for (auto& p : params) { _write << p; }
            _write.array_pop();

            _write.array_pop();
            _write.flush();

            return true;
        } catch (std::exception& e) {
            return false;
        }
    }

    bool send_reply_result(int msgid, refl::object_const_view_t retval) noexcept override
    {
        try {
            _write.array_push(4);
            _write << msgtype::reply;
            _write << msgid;

            _write << nullptr;  // error is null
            _write << retval;   // reply object

            _write.array_pop();
            _write.flush();

            return true;
        } catch (std::exception& e) {
            return false;
        }
    }

    bool send_reply_error(int msgid, refl::object_const_view_t error) noexcept override
    {
        try {
            _write.array_push(4);
            _write << msgtype::reply;
            _write << msgid;

            _write << error;    // send error object
            _write << nullptr;  // result slot is null

            _write.array_pop();
            _write.flush();

            return true;
        } catch (std::exception& e) {
            return false;
        }
    }

    bool send_reply_error(int msgid, std::string_view content) noexcept override
    {
        try {
            _write.array_push(4);
            _write << msgtype::reply;
            _write << msgid;

            _write << content;  // send error string
            _write << nullptr;  // result slot is null

            _write.array_pop();
            _write.flush();

            return true;
        } catch (std::exception& e) {
            return false;
        }
    }
};

}  // namespace CPPHEADERS_NS_::rpc::protocol
