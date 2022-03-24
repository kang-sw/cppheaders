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
#include <system_error>

#include "../../__namespace__"

namespace CPPHEADERS_NS_::rpc {
using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;

class basic_context;
class session;
struct session_profile;

using error_code = std::error_code;

enum class error_type {
    okay,
    aborted,
    timeout,
    invalid_connection,
};

class error_category : public std::error_category
{
   public:
    const char* name() const noexcept override
    {
        return "RPC error";
    }

    std::string message(int errc) const override
    {
        switch (error_type{errc}) {
            case error_type::okay: return "No error";
            case error_type::aborted: return "RPC request aborted";
            case error_type::timeout: return "RPC synchronous request timeout";
            case error_type::invalid_connection: return "Disconnected";

            default: return "Unknown error";
        }
    }

    static error_category* instance() noexcept
    {
        static error_category _cat;
        return &_cat;
    }
};

class system_error : public std::system_error
{
    using std::system_error::system_error;
};

auto make_error(error_type errc)
{
    return std::error_code(int(errc), *error_category::instance());
}

enum class rpc_type {
    request,
    notify,
    reply_okay,
    reply_error
};
}  // namespace CPPHEADERS_NS_::rpc
