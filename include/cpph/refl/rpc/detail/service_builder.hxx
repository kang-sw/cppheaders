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

#include "../../../memory/pool.hxx"
#include "../../detail/object_core.hxx"
#include "cpph/utility/functional.hxx"
#include "defs.hxx"
#include "interface.hxx"
#include "service.hxx"
#include "signature.hxx"

namespace cpph::rpc {
/**
 * Builds service
 */
class service_builder
{
    shared_ptr<service_table_t> _table;

   public:
    service_builder() noexcept = default;
    service_builder(service_builder&&) noexcept = default;
    service_builder& operator=(service_builder&&) noexcept = default;

   public:
    auto build() noexcept
    {
        service rv;
        rv._service = std::exchange(_table, nullptr);

        return rv;
    }

    void build_to(service& rv) noexcept
    {
        rv._service = std::exchange(_table, nullptr);
    }

   private:
    template <typename RetVal, typename... Params>
    class handler_impl_type : public if_service_handler
    {
        using parameter_type = tuple<std::decay_t<Params>...>;
        using param_desc_buffer_type = std::array<refl::object_view_t, sizeof...(Params)>;
        using pool_ret_type = std::conditional_t<std::is_void_v<RetVal>, nullptr_t, RetVal>;
        using handler_type = function<void(session_profile_view, RetVal*, Params...)>;

        struct param_buf_pack_t {
            parameter_type params;
            param_desc_buffer_type view_buffer;

            param_buf_pack_t() noexcept
            {
                auto fn_assign_descriptors
                        = [this](auto&... arg) {
                              size_t n = 0;
                              ((view_buffer[n++] = refl::object_view_t{arg}), ...);
                          };

                std::apply(fn_assign_descriptors, params);
            }

            // As view_buffer refers to local pointer, copying object may cause dangling.
            param_buf_pack_t(param_buf_pack_t const&) = delete;
            param_buf_pack_t& operator=(param_buf_pack_t const&) = delete;
        };

        weak_ptr<void> _owner;
        handler_type _handler;
        pool<param_buf_pack_t> _pool_param;
        pool<pool_ret_type> _pool_retval;

       public:
        handler_impl_type(weak_ptr<void> owner, handler_type fn)
                : _owner(move(owner)), _handler(move(fn)) {}

        handler_package_type checkout_parameter_buffer() override
        {
            auto data_ptr = _pool_param.checkout().share();
            auto rv = handler_package_type{};
            rv._self = shared_ptr<if_service_handler>(_owner.lock(), this);
            rv._param_body = data_ptr;
            rv.params = data_ptr->view_buffer;

            return rv;
        }

       private:
        refl::shared_object_ptr
        invoke(const session_profile& profile,
               if_service_handler::handler_package_type&& params) override
        {
            auto rv = _pool_retval.checkout();
            auto param_buf = static_cast<param_buf_pack_t*>(params._param_body.get());
            auto fn_invoke_handler =
                    [&](auto&... params) {
                        _handler(&profile, &*rv, params...);
                    };
            std::apply(fn_invoke_handler, param_buf->params);
            return refl::shared_object_ptr{move(rv).share()};
        }
    };

   public:
    template <typename RetVal, typename... Params>
    service_builder& route(
            string method_name,
            function<void(session_profile_view, RetVal*, Params...)>&& handler)
    {
        if (not _table) { _table = make_shared<service_table_t>(); }

        auto [iter, is_new] = _table->try_emplace(
                move(method_name),
                std::make_shared<handler_impl_type<RetVal, Params...>>(_table, move(handler)));

        if (not is_new) { throw std::logic_error{"method name duplication: " + method_name}; }
        return *this;
    }

    template <typename RetVal, typename... Params,
              typename Signature = signature_t<RetVal, Params...>>
    service_builder& route(
            signature_t<RetVal, Params...> const&,
            typename Signature::guide_t,
            std::false_type);

    template <typename RetVal, typename... Params,
              typename Signature = signature_t<RetVal, Params...>>
    service_builder& route(
            signature_t<RetVal, Params...> const& signature,
            typename Signature::serve_signature_full&& handler)
    {
        return route(signature.name(), move(handler));
    }

    template <typename RetVal, typename... Params, typename Callable,
              typename = std::enable_if_t<not std::is_convertible_v<
                      Callable, typename signature_t<RetVal, Params...>::serve_signature_full>>>
    service_builder& route(
            signature_t<RetVal, Params...> const& signature,
            Callable&& handler)
    {
        using signature_type = signature_t<RetVal, Params...>;

        auto func = [this, handler = std::forward<Callable>(handler)]  //
                (session_profile_view, RetVal * rbuf, auto&&... args) mutable -> void {
            if constexpr (signature_type::template invocable_1<Callable>) {
                handler(rbuf, args...);
            } else if constexpr (signature_type::template invocable_0<Callable>) {
                if constexpr (std::is_void_v<RetVal>)
                    handler(args...);
                else
                    *rbuf = handler(args...);
            } else {
                RetVal::INVALID_CALLABLE_TYPE();
            }
        };

        return route(signature.name(), signature.wrap(std::move(func)));
    }
};

}  // namespace cpph::rpc