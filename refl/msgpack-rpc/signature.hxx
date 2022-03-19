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

#include "../__namespace__"
#include "errors.hxx"

namespace CPPHEADERS_NS_::msgpack::rpc {

inline std::string_view to_string(rpc_status s)
{
    static std::map<rpc_status, std::string const, std::less<>> etos{
            {rpc_status::okay, "OKAY"},
            {rpc_status::waiting, "WAITING"},
            {rpc_status::aborted, "ABORTED"},
            {rpc_status::timeout, "ERROR_TIMEOUT"},
            {rpc_status::unknown_error, "UNKNOWN"},
            {rpc_status::internal_error, "ERROR_INTERNAL"},
            {rpc_status::invalid_parameter, "ERROR_INVALID_PARAMETER"},
            {rpc_status::invalid_return_type, "ERROR_INVALID_RETURN_TYPE"},
            {rpc_status::method_not_exist, "ERROR_METHOD_NOT_EXIST"},
    };

    auto p = find_ptr(etos, s);
    return p ? std::string_view(p->second) : std::string_view("UNKNOWN");
}

inline auto from_string(std::string_view s)
{
    static std::map<std::string, rpc_status, std::less<>> stoe{
            {"OKAY", rpc_status::okay},
            {"WAITING", rpc_status::waiting},
            {"ERROR_TIMEOUT", rpc_status::timeout},
            {"ABORTED", rpc_status::aborted},
            {"UNKOWN", rpc_status::unknown_error},
            {"ERROR_INTERNAL", rpc_status::internal_error},
            {"ERROR_INVALID_PARAMETER", rpc_status::invalid_parameter},
            {"ERROR_INVALID_RETURN_TYPE", rpc_status::invalid_return_type},
            {"ERROR_METHOD_NOT_EXIST", rpc_status::method_not_exist},
    };

    auto p = find_ptr(stoe, s);
    return p ? p->second : rpc_status::unknown_error;
}

struct session_profile;

struct rpc_error : std::runtime_error
{
    rpc_status error_code;
    explicit rpc_error(rpc_status v) noexcept
            : runtime_error(std::string(to_string(v))), error_code(v) {}
};

template <typename Signature_>
struct _function_decompose;

template <typename Ret_, typename... Args_>
struct _function_decompose<Ret_(Args_...)>
{
    using return_type = Ret_;
    using parameter_tuple_type = std::tuple<Args_...>;
};

template <size_t, typename, typename>
class signature_t;

template <size_t N_, typename RetVal_, typename... Params_>
class signature_t<N_, RetVal_, std::tuple<Params_...>>
{
   public:
    using return_type = RetVal_;
    using rpc_signature = function<void(RetVal_*, Params_...)>;
    using serve_signature = function<RetVal_(Params_&...)>;
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
        RpcContext_* const       _rpc;

       public:
        rpc_status rpc(return_type* ret, Params_ const&... args) const
        {
            return _rpc->rpc(ret, _host->name(), args...);
        }

        template <typename Duration_>
        rpc_status rpc(return_type* ret, Params_ const&... args, Duration_&& timeout) const
        {
            return _rpc->rpc(ret, _host->name(), timeout, args...);
        }

        template <typename CompletionContext_>
        auto async_rpc(return_type* ret, Params_ const&... args, CompletionContext_&& complete_handler) const
        {
            return _rpc->async_rpc(
                    ret, _host->name(),
                    std::forward<CompletionContext_>(complete_handler),
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

        auto async_rpc(return_type* ret, Params_ const&... args) const
        {
            return _rpc->async_rpc(
                    ret, _host->name(),
                    [](auto) {},
                    args...);
        }

        void notify_one(Params_ const&... args) const
        {
            _rpc->notify_one(_host->name(), args...);
        }

        size_t notify_all(Params_ const&... args) const
        {
            return _rpc->notify_all(_host->name(), args...);
        }

        template <typename Qualify_>
        size_t notify_all(Params_ const&... args, Qualify_&& qualify_function) const
        {
            return _rpc->notify_all(_host->name(), std::forward<Qualify_>(qualify_function), args...);
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
    using decomposed = _function_decompose<Signature_>;
    using return_type = typename decomposed::return_type;
    using parameter_type = typename decomposed::parameter_tuple_type;

    return signature_t<N_, return_type, parameter_type>{name};
}
}  // namespace CPPHEADERS_NS_::msgpack::rpc