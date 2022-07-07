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

#include "../../archive/msgpack-reader.hxx"
#include "../../archive/msgpack-writer.hxx"
#include "../../detail/primitives.hxx"
#include "../detail/protocol_procedure.hxx"

namespace cpph::rpc::protocol {

class msgpack : public if_protocol_procedure
{
    enum class msgtype {
        request = 0,
        reply = 1,
        notify = 2,

        INVALID = -1
    };

    struct internal_trivial_exception : std::exception {
        protocol_stream_state state;
        explicit internal_trivial_exception(protocol_stream_state st) noexcept : state(st) {}
    };

   private:
    archive::msgpack::writer _write{nullptr, 8};
    archive::msgpack::reader _read{nullptr, 8};

    // Temporary buffer, usually receives method name temporarily
    string _buf_tmp;

   public:
    explicit msgpack(archive::archive_config const& rdconf = {},
                     archive::archive_config const& wrconf = {}) noexcept
    {
        _write.config = rdconf;
        _read.config = wrconf;
    }

    void initialize(std::streambuf* streambuf) override
    {
        _write.rdbuf(streambuf);
        _read.rdbuf(streambuf);
    }

    protocol_stream_state handle_single_message(remote_procedure_message_proxy& proxy) noexcept override
    {
        using epss = protocol_stream_state;
        auto fn_verify_recoverable =
                [](bool cond, epss on_fail) {
                    if (not cond) { throw internal_trivial_exception{on_fail}; }
                };
        auto fn_error =
                [](epss flag) { throw internal_trivial_exception{flag}; };

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
                case msgtype::reply: {
                    fn_verify_recoverable(
                            _read.elem_left() == 3,
                            epss::warning_received_invalid_format);

                    int msgid = -1;
                    _read >> msgid;

                    if (_read.is_null_next()) {
                        // third argument is null -> it's valid result
                        _read >> nullptr;

                        // let proxy retrieve single object
                        proxy.reply_result(msgid, &_read);
                    } else {
                        // third argument is non-null -> it's error
                        proxy.reply_error(msgid, &_read);

                        // skip result
                        _read >> nullptr;
                    }

                    break;
                }

                case msgtype::notify: {
                    fn_verify_recoverable(
                            _read.elem_left() == 2,
                            epss::warning_received_invalid_format);

                    auto& method = _buf_tmp;
                    _read >> method;

                    auto params = proxy.notify_parameters(method);

                    if (not params)
                        fn_error(epss::warning_received_unkown_method_name);

                    {
                        auto scope_params = _read.begin_array();

                        // Verify element count matches
                        if (_read.elem_left() != params->size())
                            fn_error(epss::warning_received_invalid_number_of_parameters);

                        // Parse parameters
                        for (auto& param : *params)
                            _read >> param;

                        _read.end_array(scope_params);
                    }
                    break;
                }

                case msgtype::request: {
                    fn_verify_recoverable(
                            _read.elem_left() == 3,
                            epss::warning_received_invalid_format);

                    service_parameter_buffer* params = nullptr;
                    int msgid = -1;

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

                    _read >> msgid;
                    _read >> method;  // read method name

                    params = proxy.request_parameters(method, msgid);

                    if (params == nullptr) {
                        fn_rep_err(errstr_method_not_found);
                        fn_error(epss::warning_received_unkown_method_name);
                    }

                    // Parse parameters
                    {
                        auto scope_params = _read.begin_array();

                        // Verify element count matches
                        if (_read.elem_left() != params->size()) {
                            fn_rep_err(errstr_invalid_parameter);
                            fn_error(epss::warning_received_invalid_number_of_parameters);
                        }

                        // Parse parameters
                        try {
                            for (auto& param : *params)
                                _read >> param;
                        } catch (archive::error::reader_recoverable_exception&) {
                            fn_rep_err(errstr_invalid_parameter);
                            fn_error(epss::warning_received_invalid_parameter_type);
                        } catch (...) {
                            throw;
                        }

                        _read.end_array(scope_params);
                    }

                    break;
                }

                default:
                    fn_error(epss::warning_received_invalid_format);
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

    bool flush() noexcept override
    {
        _write.flush();
        return true;
    }

    bool send_request(std::string_view method, int msgid, array_view<refl::object_const_view_t> params) noexcept override
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

            return true;
        } catch (std::exception& e) {
            return false;
        }
    }

    bool send_notify(std::string_view method, array_view<refl::object_const_view_t> params) noexcept override
    {
        try {
            _write.array_push(3);
            _write << msgtype::notify;
            _write << method;

            _write.array_push(params.size());
            for (auto& p : params) { _write << p; }
            _write.array_pop();
            _write.array_pop();

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

}  // namespace cpph::rpc::protocol
