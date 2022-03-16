
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
#include <map>
#include <set>
#include <utility>

#include "../../functional.hxx"
#include "../../helper/exception.hxx"
#include "../../memory/pool.hxx"
#include "../__namespace__"
#include "../archive/msgpack-reader.hxx"
#include "../archive/msgpack-writer.hxx"
#include "../detail/object_core.hxx"
#include "signature.hxx"

namespace CPPHEADERS_NS_::msgpack::rpc {
using namespace archive::msgpack;

/**
 * Information of created session.
 */
struct session_profile
{
    std::string peer_name;
};

using session_profile_view = session_profile const&;

/**
 * Defines service information
 */
class service_info
{
   public:
    class if_service_handler
    {
        size_t _n_params = 0;

       public:
        explicit if_service_handler(size_t n_params) noexcept : _n_params(n_params) {}
        virtual ~if_service_handler() = default;

        //! invoke with given parameters.
        //! @return error message if invocation failed
        //! @throw remote_handler_exception
        virtual void invoke(session_profile const&, reader&, void (*)(void*, refl::object_const_view_t), void*) = 0;

        //! number of parameters
        size_t num_params() const noexcept { return _n_params; }
    };

    using handler_table_type = std::map<std::string, std::shared_ptr<if_service_handler>, std::less<>>;

   private:
    handler_table_type _handlers;

   public:
    /**
     * Original signature of serve()
     */
    template <typename RetVal_, typename... Params_>
    service_info& serve2(std::string method_name, function<void(session_profile const&, RetVal_*, Params_...)> handler)
    {
        struct service_handler : if_service_handler
        {
            using handler_type = decltype(handler);
            using return_type  = std::conditional_t<std::is_void_v<RetVal_>, nullptr_t, RetVal_>;
            using tuple_type   = std::tuple<std::decay_t<Params_>...>;

           public:
            handler_type _handler;
            pool<tuple_type> _params;
            pool<return_type> _rv_pool;

           public:
            explicit service_handler(handler_type&& h) noexcept
                    : if_service_handler(sizeof...(Params_)),
                      _handler(std::move(h)) {}

            void invoke(
                    session_profile const& session,
                    reader& reader,
                    void (*fn_write)(void*, refl::object_const_view_t),
                    void* uobj) override
            {
                std::apply(
                        [&](auto&... params) {
                            ((reader >> params), ...);

                            pool_ptr<return_type> rval;
                            if constexpr (std::is_void_v<RetVal_>)
                            {
                                _handler(session, nullptr, params...);
                                if (fn_write) { fn_write(uobj, refl::object_const_view_t{nullptr}); }
                            }
                            else
                            {
                                rval = _rv_pool.checkout();
                                _handler(session, &*rval, params...);
                                if (fn_write) { fn_write(uobj, refl::object_const_view_t{*rval}); }
                            }
                        },
                        *_params.checkout());
            }
        };

        auto [it, is_new] = _handlers.try_emplace(
                std::move(method_name),
                std::make_shared<service_handler>(std::move(handler)));

        if (not is_new)
            throw std::logic_error{"Method name must not duplicate!"};

        return *this;
    }

    /**
     * Serve RPC service. Does not distinguish notify/request handler. If client rpc mode was
     *  notify, return value of handler will silently be discarded regardless of its return
     *  type.
     *
     * @tparam RetVal_ Must be declared as refl object.
     * @tparam Params_ Must be declared as refl object.
     * @param method_name Name of method to serve
     * @param handler RPC handler. Uses cpph::function to accept move-only signatures
     */
    template <typename RetVal_, typename... Params_>
    service_info& serve1(std::string method_name, function<void(RetVal_*, Params_...)> handler)
    {
        function<void(session_profile const&, RetVal_*, Params_...)> fn =
                [_handler = std::move(handler)]  //
                (auto&&, RetVal_* buffer, auto&&... args) mutable {
                    if constexpr (std::is_void_v<RetVal_>)
                        _handler(buffer, std::forward<decltype(args)>(args)...);
                    else
                        _handler(buffer, std::forward<decltype(args)>(args)...);
                };

        this->serve2(std::move(method_name), std::move(fn));
        return *this;
    }

    /**
     * Serve RPC service. Does not distinguish notify/request handler. If client rpc mode was
     *  notify, return value of handler will silently be discarded regardless of its return
     *  type.
     *
     * @tparam RetVal_ Must be declared as refl object.
     * @tparam Params_ Must be declared as refl object.
     * @param method_name Name of method to serve
     * @param handler RPC handler. Uses cpph::function to accept move-only signatures
     */
    template <typename RetVal_, typename... Params_>
    service_info& serve(std::string method_name, function<RetVal_(Params_...)> handler)
    {
        function<void(session_profile const&, RetVal_*, Params_...)> fn =
                [_handler = std::move(handler)]  //
                (auto&&, RetVal_* buffer, auto&&... args) mutable {
                    if constexpr (std::is_void_v<RetVal_>)
                        _handler(std::forward<decltype(args)>(args)...);
                    else
                        *buffer = _handler(std::forward<decltype(args)>(args)...);
                };

        this->serve2(std::move(method_name), std::move(fn));
        return *this;
    }

    //    template <size_t N_, typename Ret_, typename... Params_, typename Callable_>
    //    service_info& operator()(
    //            signature_t<N_, Ret_, std::tuple<Params_...>> const& iface,
    //            Callable_&& service)
    //    {
    //        using iface_t    = std::decay_t<decltype(iface)>;
    //        using callable_t = std::decay_t<decltype(service)>;
    //
    //        std::string name{iface.name()};
    //
    //        if constexpr (std::is_constructible_v<typename iface_t::serve_signature, callable_t>)
    //            return serve(name, typename iface_t::serve_signature{std::forward<Callable_>(service)});
    //        if constexpr (std::is_constructible_v<typename iface_t::serve_signature_1, callable_t>)
    //            return serve1(name, typename iface_t::serve_signature_1{std::forward<Callable_>(service)});
    //        if constexpr (std::is_constructible_v<typename iface_t::serve_signature_2, callable_t>)
    //            return serve2(name, typename iface_t::serve_signature_2{std::forward<Callable_>(service)});
    //    }

    template <size_t N_, typename Ret_, typename... Params_,
              typename Signature_ = signature_t<N_, Ret_, std::tuple<Params_...>>>
    service_info& serve(
            signature_t<N_, Ret_, std::tuple<Params_...>> const& iface,
            typename Signature_::serve_signature_2 func)
    {
        return serve2(std::string(iface.name()), std::move(func));
    }

    template <size_t N_, typename Ret_, typename... Params_,
              typename Signature_ = signature_t<N_, Ret_, std::tuple<Params_...>>>
    service_info& serve(
            signature_t<N_, Ret_, std::tuple<Params_...>> const& iface,
            typename Signature_::serve_signature_1 func)
    {
        return serve1(std::string(iface.name()), std::move(func));
    }

    template <size_t N_, typename Ret_, typename... Params_,
              typename Signature_ = signature_t<N_, Ret_, std::tuple<Params_...>>>
    service_info& serve(
            signature_t<N_, Ret_, std::tuple<Params_...>> const& iface,
            typename Signature_::serve_signature func)
    {
        return serve(std::string(iface.name()), std::move(func));
    }

   public:
    auto const& _services_() const noexcept { return _handlers; }
};

// inline void pewpew()
//{
//     static auto const stub = create_signature<bool(int, double)>("absc");
//     service_info t;
//
//     t(stub, [](int, double) -> bool { return false; });
//     t(stub, [](bool*, int, double) {});
//     t(stub, [](auto&&, bool*, int, double) {});
// }

}  // namespace CPPHEADERS_NS_::msgpack::rpc