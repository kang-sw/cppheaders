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
#include <cassert>
#include <cstddef>
#include <functional>

#include "template_utils.hxx"
//
#include "__namespace__"

namespace CPPHEADERS_NS_ {

//
constexpr int _function_size = sizeof(std::function<void()>) + 16;

struct default_function_t {
    // avoids ambiguous function call error
    // function f; f = {};
    constexpr explicit default_function_t(nullptr_t) {}
};

constexpr default_function_t default_function{nullptr};

template <typename Signature_>
class function;

template <typename Ret_, typename... Args_>
class function<Ret_(Args_...)>
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
    template <typename Callable_>
    void _assign_function(Callable_&& fn)
    {
        using callable_type = std::remove_cv_t<std::remove_reference_t<Callable_>>;

        struct _callable_impl_t : _callable_t {
            Ret_ operator()(Args_... args) override
            {
                return std::invoke(_body, std::forward<Args_>(args)...);
            }

            explicit _callable_impl_t(Callable_&& fwd)
                    : _body(std::forward<Callable_>(fwd))
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
            _callable = new (_sbob()) _callable_impl_t{std::forward<Callable_>(fn)};
        } else {  // apply SBO
            _callable = new _callable_impl_t{std::forward<Callable_>(fn)};
        }
    }

    void _move_from(function&& rhs) noexcept
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
    function& operator=(function const& fn) noexcept = delete;
    function(function const& fn) noexcept = delete;

    function(default_function_t) noexcept
    {
        _assign_function(&_default_fn);
    }

    function& operator=(default_function_t) noexcept
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
            typename Callable_,
            typename = std::enable_if_t<
                    not std::is_same_v<
                            function,
                            std::remove_cv_t<std::remove_reference_t<Callable_>>>>,
            typename = std::enable_if_t<
                    std::is_invocable_r_v<Ret_, Callable_, Args_...>>>
    function& operator=(Callable_&& fn) noexcept(std::is_nothrow_move_constructible_v<Callable_>)
    {
        _destroy();
        _assign_function(std::forward<Callable_>(fn));
        return *this;
    }

    function& operator=(function&& fn) noexcept
    {
        _destroy();
        _move_from(std::move(fn));
        return *this;
    }

    template <
            typename Callable_,
            typename = std::enable_if_t<
                    not std::is_same_v<
                            function,
                            std::remove_cv_t<std::remove_reference_t<Callable_>>>>,
            typename = std::enable_if_t<
                    std::is_invocable_r_v<Ret_, Callable_, Args_...>>>
    function(Callable_&& fn) noexcept(std::is_nothrow_move_constructible_v<Callable_>)
    {
        _assign_function(std::forward<Callable_>(fn));
    }

    operator bool() const noexcept
    {
        return _callable;
    }

    function(function&& fn) noexcept
    {
        _move_from(std::move(fn));
    }

    function() noexcept = default;

    ~function() noexcept
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
        static_assert(sizeof(function) == _function_size);
    }

   private:
    _callable_t*                                     _callable = nullptr;
    std::array<char, _function_size - sizeof(void*)> _sbo_buf = {};
};

// Function utiltiies

#if __cplusplus > 201703L
using std::bind_front;
#else

// Just forward STL bind
using std::bind;

template <typename Callable_, typename Tuple_>
class _bound_functor_t
{
    Callable_ fn;
    Tuple_    captures;

   public:
    _bound_functor_t(Callable_&& fn, Tuple_&& captures)
            : fn(std::forward<Callable_>(fn)), captures(std::forward<Tuple_>(captures)) {}

   private:
    template <typename... Captures_, typename... Args_>
    constexpr auto _fn_invoke_result(std::tuple<Captures_...> const&, Args_&&...) const
            -> std::invoke_result_t<Callable_, Captures_..., Args_...>;

    auto _get_forwarded() const noexcept
    {
        return std::apply(
                [](auto&&... args) { return std::forward_as_tuple(std::forward<decltype(args)>(args)...); },
                captures);
    }

    auto _get_forwarded() noexcept
    {
        return std::apply(
                [](auto&&... args) { return std::forward_as_tuple(std::forward<decltype(args)>(args)...); },
                captures);
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
        auto tuple = std::tuple_cat(
                this->_get_forwarded(),
                std::forward_as_tuple(std::forward<decltype(args)>(args)...));

        if constexpr (std::is_same_v<void, decltype(std::apply(fn, tuple))>)
            std::apply(fn, tuple);
        else
            return std::apply(fn, tuple);
    }
};

/**
 * Bind callable with arguments in front of parameter list.
 * Function parameters will be delivered to backward
 *
 * @tparam Callable_
 * @tparam Captures_
 * @return
 */
template <class Callable_, typename... Captures_>
auto bind_front(Callable_ callable, Captures_&&... captures)
{
    return _bound_functor_t{
            std::move(callable),
            std::make_tuple(std::forward<Captures_>(captures)...)};
}
#endif

template <typename Callable_>
class _bound_weak_functor_t
{
    std::weak_ptr<void> ptr;
    Callable_           fn;

   public:
    _bound_weak_functor_t(std::weak_ptr<void> ptr, Callable_&& fn)
            : ptr(std::move(ptr)),
              fn(std::forward<Callable_>(fn)) {}

    template <typename... Args_>
    auto operator()(Args_&&... args)
            -> std::invoke_result_t<Callable_, Args_...>
    {
        auto tuple = std::forward_as_tuple(std::forward<Args_>(args)...);
        enum : bool { return_void = std::is_void_v<std::invoke_result_t<Callable_, Args_...>> };

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
            -> std::invoke_result_t<Callable_, Args_...>
    {
        auto tuple = std::forward_as_tuple(std::forward<Args_>(args)...);
        enum : bool { return_void = std::is_void_v<std::invoke_result_t<Callable_, Args_...>> };

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
};

/**
 * Bind front callable with weak reference. Actual callable will be invoked
 *  only when given weak reference is valid.
 */
template <class Callable_, typename Ptr_, typename... Captures_>
auto bind_front_weak(Ptr_&& ref, Callable_ callable, Captures_&&... captures)
{
    using std::decay_t;
    return _bound_weak_functor_t{
            std::forward<Ptr_>(ref),
            bind_front(std::move(callable), std::forward<Captures_>(captures)...)};
}

/**
 * Bind function with weak pointer
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
}  // namespace CPPHEADERS_NS_
