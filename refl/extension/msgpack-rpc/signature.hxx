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
#include <tuple>

#include "../../__namespace__"
#include "errors.hxx"

namespace CPPHEADERS_NS_::msgpack::rpc {
struct session_profile;

struct rpc_error : std::exception
{
    rpc_status error_code;
    explicit rpc_error(rpc_status v) noexcept : error_code(v) {}
};

template <typename Signature_>
struct _function_decompose;

template <typename Ret_, typename... Args_>
struct _function_decompose<Ret_(Args_...)>
{
    using return_type          = Ret_;
    using parameter_tuple_type = std::tuple<Args_...>;
};

template <size_t, typename, typename>
class signature_t;

template <size_t N_, typename RetVal_, typename... Params_>
class signature_t<N_, RetVal_, std::tuple<Params_...>>
{
   public:
    using return_type       = RetVal_;
    using rpc_signature     = function<void(RetVal_*, Params_...)>;
    using serve_signature   = function<RetVal_(Params_&...)>;
    using serve_signature_1 = function<void(RetVal_*, Params_&...)>;
    using serve_signature_2 = function<void(session_profile const&, RetVal_*, Params_&...)>;

   private:
    char _method_name[N_]{};

   public:
    explicit signature_t(char const (&name)[N_]) noexcept
    {
        std::copy_n(name, N_, _method_name);
    }

    std::string_view name() const noexcept { return std::string_view(_method_name, N_ - 1); }

   public:
    template <class RpcContext_>
    struct invoke_proxy_t
    {
        signature_t const* const _host;
        RpcContext_* const _rpc;

       public:
        rpc_status rpc(return_type* ret, Params_ const&... args) const
        {
            return _rpc->rpc(ret, _host->name(), args...);
        }

        void notify(Params_ const&... args) const
        {
            _rpc->notify(_host->name(), args...);
        }

        void notify_all(Params_ const&... args) const
        {
            _rpc->notify_all(_host->name(), args...);
        }

        return_type operator()(Params_ const&... args) const
        {
            rpc_status result;

            if constexpr (std::is_void_v<return_type>)
            {
                result = _rpc->rpc(nullptr, _host->name(), args...);
                if (result != rpc_status::okay) { throw remote_invoke_exception(result); }
            }
            else
            {
                return_type retval;

                result = _rpc->rpc(&retval, _host->name(), args...);
                if (result != rpc_status::okay) { throw remote_invoke_exception(result); }

                return retval;
            }
        }
    };

    template <class RpcContext_>
    auto operator()(RpcContext_& rpc) const
    {
        return invoke_proxy_t<RpcContext_>{this, &rpc};
    }
};

template <typename Signature_, size_t N_>
constexpr auto create_signature(char const (&name)[N_])
{
    using decomposed     = _function_decompose<Signature_>;
    using return_type    = typename decomposed::return_type;
    using parameter_type = typename decomposed::parameter_tuple_type;

    return signature_t<N_, return_type, parameter_type>{name};
}
}  // namespace CPPHEADERS_NS_::msgpack::rpc