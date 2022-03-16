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
#include <map>
#include <string>

#include "../../../helper/exception.hxx"
#include "../../__namespace__"
#include "defs.hxx"

namespace CPPHEADERS_NS_::msgpack::rpc {
CPPH_DECLARE_EXCEPTION(exception, std::exception);
CPPH_DECLARE_EXCEPTION(invalid_connection, exception);

//! Exception propagated to RPC client.
CPPH_DECLARE_EXCEPTION(remote_reply_exception, std::runtime_error);

//!
class remote_handler_exception : public std::exception
{
    std::any _body;
    refl::object_const_view_t _view;

   public:
    template <typename ArgTy_,
              typename = std::enable_if_t<
                      not std::is_same_v<std::decay_t<ArgTy_>, remote_handler_exception>>>
    explicit remote_handler_exception(ArgTy_&& other)
    {
        using value_type = std::decay_t<ArgTy_>;
        _body            = std::make_any<value_type>(std::forward<ArgTy_>(other));
        auto& ref        = std::any_cast<value_type&>(_body);
        _view            = refl::object_const_view_t{ref};
    }

    refl::object_const_view_t view() const { return _view; }
};

namespace detail {
CPPH_DECLARE_EXCEPTION(rpc_handler_error, exception);
CPPH_DECLARE_EXCEPTION(rpc_handler_missing_parameter, rpc_handler_error);
CPPH_DECLARE_EXCEPTION(rpc_handler_fatal_state, rpc_handler_error);
}  // namespace detail
}  // namespace CPPHEADERS_NS_::msgpack::rpc
