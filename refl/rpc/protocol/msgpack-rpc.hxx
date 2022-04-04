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
        notify = 2,

        INVALID = 999
    };

    struct internal_trivial_exception : std::exception {
        protocol_stream_state state;
        explicit internal_trivial_exception(protocol_stream_state st) noexcept : state(st) {}
    };

   private:
    archive::msgpack::writer _write{nullptr, 8};
    archive::msgpack::reader _read{nullptr, 8};

    string                   _buf_tmp;

   public:
    void initialize(if_connection_streambuf* streambuf) override
    {
        _write.rdbuf(streambuf);
        _read.rdbuf(streambuf);
    }

    protocol_stream_state handle_single_message(remote_procedure_message_proxy& proxy) noexcept override
    {
        using epss = protocol_stream_state;
        static auto verify_recoverable =
                [](bool cond, epss on_fail) {
                    if (not cond)
                        throw internal_trivial_exception{on_fail};
                };

        archive::context_key scope = {};

        try {
            scope = _read.begin_array();
        } catch (std::exception&) {
            return epss::expired;
        }

        try {
            msgtype type = msgtype::INVALID;
            _read >> type;

            switch (type) {
                break;

                case msgtype::reply: {
                    verify_recoverable(
                            _read.elem_left() == 3,
                            epss::warning_received_invalid_format);

                    // TODO
                    break;
                }

                case msgtype::request:
                case msgtype::notify: {
                    bool const is_request = type == msgtype::request;
                    verify_recoverable(
                            _read.elem_left() == (is_request ? 3 : 2),
                            epss::warning_received_invalid_format);

                    service_parameter_buffer* params = nullptr;
                    int                       msgid = -1;

                    // Reply error, only when it's rpc
                    auto fn_rep_err =
                            [&](auto&& errcontent) {
                                assert(type == msgtype::request);

                                _write.array_push(4);
                                _write << msgtype::reply;
                                _write << msgid;
                                _write << errcontent;
                                _write << nullptr;
                                _write.array_pop();
                            };

                    auto& method = _buf_tmp;
                    _read >> method;  // read method name

                    if (is_request) {
                        _read >> msgid;
                        params = proxy.request_parameters(method, msgid);

                        if (params == nullptr) {
                            fn_rep_err(errstr_method_not_found);
                            verify_recoverable(false, epss::warning_received_unkown_method_name);
                        }
                    } else {
                        params = proxy.notify_parameters(method);
                        verify_recoverable(params, epss::warning_received_unkown_method_name);
                    }

                    // Parse parameters
                    {
                        auto scope_params = _read.begin_array();

                        // Verify element count matches
                        if (_read.elem_left() != params->size()) {
                            if (is_request)
                                fn_rep_err(errstr_invalid_parameter);

                            verify_recoverable(false, epss::warning_received_invalid_number_of_parameters);
                        }

                        // Parse parameters
                        for (auto& p : *params)
                            _read >> p;

                        _read.end_array(scope_params);
                    }

                    break;
                }

                default:
                    verify_recoverable(false, epss::warning_received_invalid_format);
            }

            _read.end_array(scope);
            return epss::okay;
        } catch (internal_trivial_exception& e) {
            _read.end_array(scope);
            return e.state;
        } catch (archive::error::reader_recoverable_exception&) {
            _read.end_array(scope);
            return epss::warning_unknown;
        } catch (...) {
            return epss::expired;
        }
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
