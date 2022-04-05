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
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#include "../../__namespace__"
#include "../../detail/object_core.hxx"

namespace CPPHEADERS_NS_::rpc {
using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::weak_ptr;

struct session_profile;
using session_profile_view = session_profile const*;

class basic_context;
class session;

using error_code = std::error_code;

/**
 * Type of errors can occur
 */
enum class request_result {
    okay = 0,

    aborted,
    timeout,
    invalid_connection,

    exception_returned,
};

class request_error_category : public std::error_category
{
   public:
    const char* name() const noexcept override
    {
        return "RPC error";
    }

    std::string message(int errc) const override
    {
        switch (request_result{errc}) {
            case request_result::okay: return "No error";
            case request_result::aborted: return "RPC request aborted";
            case request_result::timeout: return "RPC synchronous request timeout";
            case request_result::invalid_connection: return "This connection is expired";
            case request_result::exception_returned: return "Remote handler returned exception";

            default: return "Unknown error";
        }
    }

    static request_error_category* instance() noexcept
    {
        static request_error_category _cat;
        return &_cat;
    }
};

auto make_request_error(request_result errc)
{
    return std::error_code(int(errc), *request_error_category::instance());
}

class request_exception : public std::system_error
{
   public:
    string content;

   public:
    using std::system_error::system_error;

    explicit request_exception(request_result errc, std::string* str = {})
            : std::system_error(make_request_error(errc)),
              content(str ? std::string{std::move(*str)} : std::string{}) {}
};

/**
 * Defines current protocol stream state after operation
 */
enum class protocol_stream_state {
    okay = 0,
    expired = -1,  // Protocol is in irreversible state. Session must be disposed.

    // Errors that are '> recoverable_errors' are just warnings.
    _warnings_ = 1,
    warning_unknown,
    warning_received_invalid_number_of_parameters,
    warning_received_invalid_parameter_type,
    warning_received_unkown_method_name,
    warning_received_invalid_format,
    warning_received_expired_rpc,
};

/**
 *
 */
class service_handler_exception : public std::exception
{
    refl::shared_object_ptr _data;

   public:
    template <typename ExceptionArg>
    explicit service_handler_exception(ExceptionArg&& data)
            : _data(refl::shared_object_ptr(make_shared<ExceptionArg>(
                    std::forward<ExceptionArg>(data))))
    {
    }

    auto const& data() const noexcept { return _data; }
};

/**
 * Generic error strings for identifiable internal errors
 */
constexpr string_view errstr_method_not_found = "CPPH_RPC_ERROR_METHOD_NOT_FOUND",
                      errstr_invalid_parameter = "CPPH_RPC_ERROR_INVALID_PARAMETER";

}  // namespace CPPHEADERS_NS_::rpc
