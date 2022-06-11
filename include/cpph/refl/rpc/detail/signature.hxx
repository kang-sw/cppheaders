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

#include "cpph/utility/functional.hxx"
#include "defs.hxx"

namespace cpph::rpc {
using std::string, std::string_view;

template <typename Signature_>
struct _function_decompose;

template <typename Ret_, typename... Args_>
struct _function_decompose<Ret_(Args_...)> {
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
    using serve_signature_0 = function<RetVal_(Params_&...)>;
    using serve_signature_1 = function<void(RetVal_*, Params_&...)>;
    using serve_signature_full = function<void(session_profile_view, RetVal_*, Params_&...)>;

    // Helper for clion code inspection ...
    using guide_t = void (*)(session_profile_view, RetVal_*, Params_&...);
    static inline guide_t const guide = nullptr;

    template <typename Callable>
    static constexpr bool invocable_0 = std::is_invocable_r_v<RetVal_, Callable, Params_&...>;

    template <typename Callable>
    static constexpr bool invocable_1 = std::is_invocable_v<Callable, RetVal_*, Params_&...>;

   private:
    string const _method_name;

   public:
    explicit signature_t(string name) noexcept : _method_name(std::move(name)) {}
    string const& name() const noexcept { return _method_name; }

   public:
    template <class RpcContext_>
    struct invoke_proxy_t {
        signature_t const* const _host;
        RpcContext_* const _rpc;

       public:
        template <class Duration = nullptr_t>
        pair<request_result, optional<string>>
        request_with(return_type* retval, Params_ const&... args, Duration&& timeout = nullptr) const
        {
            request_result result = {};
            optional<string> errstr;

            auto fn_on_complete = [&](error_code const& ec, auto str) { result = (request_result)ec.value(), errstr.emplace(str); };
            auto handle = _rpc->async_request(_host->name(), static_cast<function<void(const error_code&, string_view)>>(fn_on_complete), retval, args...);

            assert(bool(handle) && "Handle is always valid since RPC must throw on invalid connection!");

            if constexpr (std::is_null_pointer_v<Duration>) {
                _rpc->wait(handle);
            } else if (not _rpc->wait_for(handle, std::forward<Duration>(timeout))) {
                result = request_result::timeout;
                handle.abort();
            }

            return {result, move(errstr)};
        }

        return_type request(Params_ const&... args) const
        {
            return_type retval;
            auto [ec, errstr] = this->request_with(&retval, args...);

            if (ec != request_result::okay)
                throw request_exception(ec, errstr ? &*errstr : nullptr);

            return retval;
        }

        template <class Duration>
        return_type request(Params_ const&... args, Duration&& dur) const
        {
            return_type retval;
            auto [ec, errstr] = this->request_with(&retval, args..., std::forward<Duration>(dur));

            if (ec != request_result::okay)
                throw request_exception(ec, errstr ? &*errstr : nullptr);

            return retval;
        }

        template <typename CompletionContext_>
        auto async_request(return_type* ret, Params_ const&... args, CompletionContext_&& complete_handler) const
        {
            return _rpc->async_request(
                    _host->name(),
                    std::forward<CompletionContext_>(complete_handler),
                    ret, args...);
        }

        auto async_request(return_type* ret, Params_ const&... args) const
        {
            return _rpc->async_request(
                    _host->name(),
                    default_function,
                    ret, args...);
        }

        template <typename CompletionContext_>
        auto async_request(Params_ const&... args, CompletionContext_&& complete_handler) const
        {
            return _rpc->async_request(
                    _host->name(),
                    std::forward<CompletionContext_>(complete_handler),
                    nullptr, args...);
        }

        auto async_rpc(Params_ const&... args) const
        {
            return _rpc->async_request(
                    _host->name(),
                    default_function,
                    nullptr, args...);
        }

        void notify(Params_ const&... args) const
        {
            _rpc->notify(_host->name(), args...);
        }

        template <typename Filter>
        size_t notify(Params_ const&... args, Filter&& fn) const
        {
            return _rpc->notify_filter(_host->name(), std::forward<Filter>(fn), args...);
        }
    };

    template <class Session>
    auto operator()(Session&& rpc_ptr) const
    {
        using session_type = std::decay_t<decltype(*rpc_ptr)>;
        return invoke_proxy_t<session_type>{this, &*rpc_ptr};
    }

    auto&& wrap(serve_signature_full&& signature) const noexcept { return std::move(signature); }
};

template <typename Signature_>
auto create_signature(string name)
{
    using decomposed = _function_decompose<Signature_>;
    using return_type = typename decomposed::return_type;
    using param_tuple_type = typename decomposed::parameter_tuple_type;

    return signature_t<return_type, param_tuple_type>{name};
}
}  // namespace cpph::rpc