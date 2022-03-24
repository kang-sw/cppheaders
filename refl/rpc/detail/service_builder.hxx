// MIT License
//
// Copyright (c) 2021-2022. Seungwoo Kang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// project home: https://github.com/perfkitpp

#pragma once
#include <array>
#include <map>
#include <utility>

#include "../../../functional.hxx"
#include "../../../memory/pool.hxx"
#include "../../__namespace__"
#include "../../detail/object_core.hxx"
#include "defs.hxx"
#include "interface.hxx"
#include "service.hxx"
#include "signature.hxx"

namespace CPPHEADERS_NS_::rpc {

template <typename RetVal, typename... Params>
using service_handler_fn = function<RetVal(session_profile const&, RetVal*, Params...)>;

class service_builder
{
    shared_ptr<service_table_t> _table;

   public:
    auto confirm() noexcept
    {
        service rv;
        rv._service = std::exchange(_table, nullptr);

        return rv;
    }

    template <typename RetVal, typename... Params>
    service_builder& route(
            string                                  method_name,
            service_handler_fn<RetVal, Params...>&& handler)
    {
        if (not _table) { _table = make_shared<service_table_t>(); }

        class handler_impl_t : public if_service_handler
        {
            using parameter_type = tuple<std::decay_t<Params>...>;
            using param_desc_buffer_type = std::array<refl::object_view_t, sizeof...(Params)>;

            struct param_buf_pack_t
            {
                parameter_type         params;
                param_desc_buffer_type view_buffer;

                param_buf_pack_t() noexcept
                {
                    auto fn_assign_descriptors
                            = [this](auto&... arg) {
                                  size_t n = 0;
                                  ((view_buffer[n++] = refl::object_view_t{&arg}), ...);
                              };

                    std::apply(fn_assign_descriptors, params);
                }
            };

            weak_ptr<void>         _owner;
            decltype(handler)      _handler;
            pool<param_buf_pack_t> _pool_param;
            pool<RetVal>           _pool_retval;

           public:
            handler_impl_t(decltype(_owner) owner, decltype(_handler))
                    : _owner(move(owner)), _handler(move(handler)) {}

            auto checkout_parameter_buffer() -> handler_package_type override
            {
                auto data_ptr = _pool_param.checkout().share();
                auto rv = handler_package_type{};
                rv._self = shared_ptr<if_service_handler>(_owner, this);
                rv._handle = data_ptr;
                rv.params = data_ptr->view_buffer;

                return rv;
            }

           private:
            auto invoke(const session_profile& profile, if_service_handler::handler_package_type&& params) -> refl::shared_object_ptr override
            {
                auto rv = _pool_retval.checkout();
                auto param_buf = static_cast<param_buf_pack_t*>(params._handle.get());
                auto fn_invoke_handler =
                        [&](auto&... params) {
                            _handler(&profile, &*rv, params...);
                        };
                std::apply(fn_invoke_handler, param_buf->params);
                return rv.share();
            }
        };

        auto [iter, is_new] = _table->try_emplace(
                move(method_name),
                std::make_unique<handler_impl_t>(_table, move(handler)));

        if (not is_new) { throw std::logic_error{"method name duplication: " + method_name}; }
        return *this;
    }

    template <typename RetVal, typename... Params,
              typename Signature = signature_t<RetVal, Params...>>
    service_builder& route(Signature const&                        signature,
                           service_handler_fn<RetVal, Params...>&& handler)
    {
        return route(signature.name(), move(handler));
    }

    template <typename RetVal, typename... Params,
              typename Signature = signature_t<RetVal, Params...>,
              typename Callable,
              typename = enable_if_t<not is_convertible_v<Callable, typename Signature::serve_signature_2>>>
    service_builder& route(Signature const& signature,
                           Callable&&       handler)
    {
        enum : bool { is_void_return = std::is_void_v<RetVal> };

        auto func = [this, handler = forward<Callable>(handler)]  //
                (auto&&, RetVal* rbuf, auto&&... args) mutable {
                    if constexpr (std::is_invocable_v<Callable, RetVal*, Params...>) {
                        handler(rbuf, args...);
                    } else if constexpr (std::is_invocable_r_v<RetVal, Callable, Params...>) {
                        if constexpr (is_void_return)
                            handler(args...);
                        else
                            *rbuf = handler(args...);
                    } else {
                        RetVal::INVALID_CALLABLE_TYPE();
                    }
                };

        return route(signature.name(), move(func));
    }
};

}  // namespace CPPHEADERS_NS_::rpc