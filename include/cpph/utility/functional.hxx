/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2021-2022. Seungwoo Kang
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
#include <cassert>
#include <cstddef>
#include <functional>

#include "generic.hxx"
//

namespace cpph {
using std::function;
using std::invoke;
using std::is_function_v;

//
constexpr int _function_size = sizeof(std::function<void()>) + 16;

struct default_function_t {
    // avoids ambiguous function call error
    // function f; f = {};
    constexpr explicit default_function_t(nullptr_t) {}
};

constexpr default_function_t default_function{nullptr};

/**
 * Non-copyable unique function
 * @tparam Signature_
 */
template <typename Signature_>
class ufunction;

template <typename Ret_, typename... Args_>
class ufunction<Ret_(Args_...)>
{
   public:
    using return_type = Ret_;
    using function_type = std::function<Ret_(Args_...)>;

   private:
    struct _callable_t {
        virtual ~_callable_t() = default;
        virtual Ret_ operator()(Args_... args) = 0;
        virtual void move(void* to) = 0;
    };

   private:
    template <typename Callable>
    void _assign_function(Callable&& fn)
    {
        using callable_type = std::remove_cv_t<std::remove_reference_t<Callable>>;

        struct _callable_impl_t : _callable_t {
            Ret_ operator()(Args_... args) override
            {
                return std::invoke(_body, std::forward<Args_>(args)...);
            }

            explicit _callable_impl_t(Callable&& fwd)
                    : _body(std::forward<Callable>(fwd))
            {
            }

            void move(void* raw) override
            {
                new (raw) _callable_impl_t(std::move(*this));
            }

           private:
            callable_type _body;
        };

        if constexpr (sizeof(_callable_impl_t) <= sizeof _sbo_buf) {  // allocate on heap
            _callable = new (_sbob()) _callable_impl_t{std::forward<Callable>(fn)};
        } else {  // apply SBO
            _callable = new _callable_impl_t{std::forward<Callable>(fn)};
        }
    }

    void _move_from(ufunction&& rhs) noexcept
    {
        if (not rhs.is_sbo()) {
            _callable = rhs._callable;
            rhs._callable = nullptr;
        } else {
            _callable = (_callable_t*)_sbob();
            rhs._callable->move(_sbob());
            rhs._destroy();
        }
    }

   private:
    static Ret_ _default_fn(Args_...)
    {
        if constexpr (not std::is_same_v<void, Ret_>)
            return Ret_{};
    }

   public:
    ufunction& operator=(ufunction const& fn) noexcept = delete;
    ufunction(ufunction const& fn) noexcept = delete;

    ufunction(default_function_t) noexcept
    {
        _assign_function(&_default_fn);
    }

    ufunction& operator=(default_function_t) noexcept
    {
        _destroy();
        _assign_function(&_default_fn);
        return *this;
    }

    Ret_ operator()(Args_... args) const
    {
        assert(_callable != nullptr);
        return std::invoke(*_callable, std::forward<Args_>(args)...);
    }

    template <
            typename Callable,
            typename = std::enable_if_t<
                    not std::is_same_v<
                            ufunction,
                            std::remove_cv_t<std::remove_reference_t<Callable>>>>,
            typename = std::enable_if_t<
                    std::is_invocable_r_v<Ret_, Callable, Args_...>>>
    ufunction& operator=(Callable&& fn) noexcept(std::is_nothrow_move_constructible_v<Callable>)
    {
        _destroy();
        _assign_function(std::forward<Callable>(fn));
        return *this;
    }

    ufunction& operator=(ufunction&& fn) noexcept
    {
        _destroy();
        _move_from(std::move(fn));
        return *this;
    }

    template <
            typename Callable,
            typename = std::enable_if_t<
                    not std::is_same_v<
                            ufunction,
                            std::remove_cv_t<std::remove_reference_t<Callable>>>>,
            typename = std::enable_if_t<
                    std::is_invocable_r_v<Ret_, Callable, Args_...>>>
    ufunction(Callable&& fn) noexcept(std::is_nothrow_move_constructible_v<Callable>)
    {
        _assign_function(std::forward<Callable>(fn));
    }

    operator bool() const noexcept
    {
        return _callable;
    }

    ufunction(ufunction&& fn) noexcept
    {
        _move_from(std::move(fn));
    }

    ufunction() noexcept = default;

    ~ufunction() noexcept
    {
        _destroy();
    }

    bool is_sbo() const noexcept
    {
        return _is_sbo();
    }

   private:
    void _destroy()
    {
        if (_is_sbo())
            (*_callable).~_callable_t();
        else
            delete _callable;

        _callable = nullptr;
    }

    bool _is_sbo() const noexcept
    {
        return (void*)_callable == (void*)_sbob();
    }

    void* _sbob() const noexcept
    {
        return const_cast<char*>(_sbo_buf.data());
    }

    void _static_assert_expr() const noexcept
    {
        static_assert(sizeof _sbo_buf == _function_size - sizeof(void*));
        static_assert(sizeof(ufunction) == _function_size);
    }

   private:
    _callable_t* _callable = nullptr;
    std::array<char, _function_size - sizeof(void*)> _sbo_buf = {};
};

// Function utiltiies

#if __cplusplus > 201703L
using std::bind_front;
#else

// Just forward STL bind
using std::bind;

template <typename Callable, typename Tuple_>
class _bound_functor_t
{
    Callable fn;
    Tuple_ captures;

   public:
    _bound_functor_t(Callable&& fn, Tuple_&& captures)
            : fn(std::forward<Callable>(fn)), captures(std::forward<Tuple_>(captures)) {}

   private:
    template <typename... Captures_, typename... Args_>
    constexpr auto _fn_invoke_result(std::tuple<Captures_...> const&, Args_&&...) const
            -> std::invoke_result_t<Callable, Captures_..., Args_...>;

    auto _get_forwarded() noexcept
    {
        return std::apply(
                [](auto&&... args) { return std::forward_as_tuple(std::forward<decltype(args)>(args)...); },
                captures);
    }

    auto _get_forwarded() const noexcept
    {
        return self_non_const_()._get_forwarded();
    }

   public:
    template <typename... Args_>
    auto operator()(Args_&&... args)
            -> decltype(_fn_invoke_result(captures, std::forward<Args_>(args)...))
    {
        auto tuple = std::tuple_cat(
                this->_get_forwarded(),
                std::forward_as_tuple(std::forward<decltype(args)>(args)...));

        if constexpr (std::is_same_v<void, decltype(std::apply(fn, tuple))>)
            std::apply(fn, tuple);
        else
            return std::apply(fn, tuple);
    }

    template <typename... Args_>
    auto operator()(Args_&&... args) const
            -> decltype(_fn_invoke_result(captures, std::forward<Args_>(args)...))
    {
        return self_non_const_().operator()(forward<Args_>(args)...);
    }

   private:
    _bound_functor_t& self_non_const_() const noexcept
    {
        return const_cast<_bound_functor_t&>(*this);
    }
};

/**
 * Bind callable with arguments in front of parameter list.
 * Function parameters will be delivered to backward
 *
 * @tparam Callable
 * @tparam Captures_
 * @return
 */
template <class Callable, typename... Captures_>
auto bind_front(Callable callable, Captures_&&... captures)
{
    return _bound_functor_t{
            std::move(callable),
            std::make_tuple(std::forward<Captures_>(captures)...)};
}
#endif

template <typename Callable>
class _bound_weak_functor_t
{
    std::weak_ptr<void> ptr;
    Callable fn;

   public:
    _bound_weak_functor_t(std::weak_ptr<void> ptr, Callable&& fn)
            : ptr(std::move(ptr)),
              fn(std::forward<Callable>(fn)) {}

    template <typename... Args_>
    auto operator()(Args_&&... args)
            -> std::invoke_result_t<Callable, Args_...>
    {
        auto tuple = std::forward_as_tuple(std::forward<Args_>(args)...);
        enum : bool { return_void = std::is_void_v<std::invoke_result_t<Callable, Args_...>> };

        if (auto anchor = ptr.lock()) {
            if constexpr (return_void)
                std::apply(fn, tuple);
            else
                return std::apply(fn, tuple);
        } else {
            if constexpr (return_void)
                ;
            else
                return {};
        }
    }

    template <typename... Args_>
    auto operator()(Args_&&... args) const
            -> std::invoke_result_t<Callable, Args_...>
    {
        return const_cast<_bound_weak_functor_t&>(*this).operator()(std::forward<Args_>(args)...);
    }
};

/**
 * Bind front callable with weak reference. Actual callable will be invoked
 *  only when given weak reference is valid.
 */
template <class Callable, typename Ptr_, typename... Captures_>
auto bind_front_weak(Ptr_&& ref, Callable callable, Captures_&&... captures)
{
    using std::decay_t;
    return _bound_weak_functor_t{
            std::forward<Ptr_>(ref),
            bind_front(std::move(callable), std::forward<Captures_>(captures)...)};
}

/**
 * Bind ufunction with weak pointer
 */
template <class Callable, typename Ptr, typename... Captures,
          typename = std::enable_if_t<std::is_invocable_v<Callable, Captures...>>>
auto bind_weak(Ptr&& ref, Callable callable, Captures&&... captures)
{
    return [ref = std::weak_ptr(ref),
            callable = std::bind(std::forward<Callable>(callable), std::forward<Captures>(captures)...)] {
        using result_type = std::invoke_result_t<decltype(callable)>;
        enum { non_void_return = not std::is_void_v<result_type> };

        if (auto lock = ref.lock()) {
            if constexpr (non_void_return)
                return callable();
            else
                callable();
        } else if constexpr (non_void_return) {
            return result_type{};
        }
    };
}

template <class Callable>
struct _heap_bound_functor_t {
    shared_ptr<Callable> callable_;

    template <typename... Params>
    auto operator()(Params&&... params)
    {
        if (is_void_v<decltype((*callable_)(forward<Params>(params)...))>)
            (*callable_)(forward<Params>(params)...);
        else
            return (*callable_)(forward<Params>(params)...);
    }

    template <typename... Params>
    auto operator()(Params&&... params) const
    {
        return const_cast<_heap_bound_functor_t&>(*this).operator()(forward<Params>(params)...);
    }
};

template <typename Callable>
auto share_callable(Callable&& shared) -> _heap_bound_functor_t<decay_t<Callable>>
{
    return {make_shared<decay_t<Callable>>(forward<Callable>(shared))};
}
}  // namespace cpph
