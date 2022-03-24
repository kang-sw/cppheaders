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
#include <array>
#include <string>
#include <tuple>

#include "../../../functional.hxx"
#include "../../__namespace__"
#include "defs.hxx"

namespace CPPHEADERS_NS_::rpc {
using std::string, std::string_view;

template <typename Signature_>
struct _function_decompose;

template <typename Ret_, typename... Args_>
struct _function_decompose<Ret_(Args_...)>
{
    using return_type = Ret_;
    using parameter_tuple_type = std::tuple<Args_...>;
};

template <typename, typename>
class signature_t;

template <typename RetVal_, typename... Params_>
class signature_t<RetVal_, std::tuple<Params_...>>
{
   public:
    using return_type = RetVal_;
    using rpc_signature = function<void(RetVal_*, Params_...)>;
    using serve_signature = function<RetVal_(Params_&...)>;
    using serve_signature_1 = function<void(RetVal_*, Params_&...)>;
    using serve_signature_2 = function<void(struct session_profile const&, RetVal_*, Params_&...)>;

   private:
    string const _method_name;

   public:
    explicit signature_t(string name) noexcept : _method_name(std::move(name)) {}
    string const& name() const noexcept { return _method_name; }

   public:
    template <class RpcContext_>
    struct invoke_proxy_t
    {
        signature_t const* const _host;
        RpcContext_* const       _rpc;

       public:
        template <typename Timeout_>
        return_type rpc(Params_ const&... args, Timeout_ timeout) const
        {
            return_type ret;
            error_type  errc = _rpc->rpc(&ret, _host->name(), timeout, args...);
            if (errc != error_type::okay) { throw system_error{make_error(errc)}; }

            return ret;
        }

        template <typename CompletionContext_>
        auto async_rpc(return_type* ret, Params_ const&... args, CompletionContext_&& complete_handler) const
        {
            return _rpc->async_rpc(
                    ret, _host->name(),
                    std::forward<CompletionContext_>(complete_handler),
                    args...);
        }

        auto async_rpc(return_type* ret, Params_ const&... args) const
        {
            return _rpc->async_rpc(
                    ret, _host->name(),
                    [](auto) {},
                    args...);
        }

        template <typename CompletionContext_>
        auto async_rpc(Params_ const&... args, CompletionContext_&& complete_handler) const
        {
            return _rpc->async_rpc(
                    nullptr, _host->name(),
                    std::forward<CompletionContext_>(complete_handler),
                    args...);
        }

        auto async_rpc(Params_ const&... args) const
        {
            return _rpc->async_rpc(
                    nullptr, _host->name(),
                    [](auto) {},
                    args...);
        }

        void notify(Params_ const&... args) const
        {
            _rpc->notify(_host->name(), args...);
        }

        template <typename Qualify_>
        size_t notify(Params_ const&... args, Qualify_&& qualify_function) const
        {
            return _rpc->notify(_host->name(), std::forward<Qualify_>(qualify_function), args...);
        }
    };

    template <class RpcContext_>
    auto operator()(RpcContext_& rpc) const
    {
        return invoke_proxy_t<RpcContext_>{this, &rpc};
    }
};

template <typename Signature_>
auto create_signature(string name)
{
    using decomposed = _function_decompose<Signature_>;
    using return_type = typename decomposed::return_type;
    using parameter_type = typename decomposed::parameter_tuple_type;

    return signature_t<return_type, parameter_type>{name};
}
}  // namespace CPPHEADERS_NS_::rpc