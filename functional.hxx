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
#include <cstddef>
#include <functional>

#include "template_utils.hxx"
//
#include "__namespace__.h"

namespace CPPHEADERS_NS_ {

//
constexpr int _function_size = sizeof(std::function<void()>) + 16;

struct default_function_t
{
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
    using return_type   = Ret_;
    using function_type = std::function<Ret_(Args_...)>;

   private:
    struct _callable_t
    {
        virtual ~_callable_t()                 = default;
        virtual Ret_ operator()(Args_... args) = 0;
        virtual void move(void* to)            = 0;
    };

   private:
    template <typename Callable_>
    void _assign_function(Callable_&& fn)
    {
        using callable_type = std::remove_cv_t<std::remove_reference_t<Callable_>>;

        struct _callable_impl_t : _callable_t
        {
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

        if constexpr (sizeof(_callable_impl_t) <= sizeof _sbo_buf)
        {  // allocate on heap
            _callable = new (_sbob()) _callable_impl_t{std::forward<Callable_>(fn)};
        }
        else
        {  // apply SBO
            _callable = new _callable_impl_t{std::forward<Callable_>(fn)};
        }
    }

    void _move_from(function&& rhs) noexcept
    {
        if (not rhs.is_sbo())
        {
            _callable     = rhs._callable;
            rhs._callable = nullptr;
        }
        else
        {
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
    function(function const& fn) noexcept            = delete;

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
                            std::remove_cv_t<std::remove_reference_t<Callable_>>>>>
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
                            std::remove_cv_t<std::remove_reference_t<Callable_>>>>>
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
    _callable_t* _callable                                    = nullptr;
    std::array<char, _function_size - sizeof(void*)> _sbo_buf = {};
};

// Function utiltiies

/**
 * Bind callable with arguments in front of parameter list.
 * Function parameters will be delivered to backward
 *
 * @tparam Callable_
 * @tparam Captures_
 * @return
 */
template <class Callable_, typename... Captures_>
auto bind_front(Callable_&& callable, Captures_&&... captures)
{
    return
            [fn       = std::forward<Callable_>(callable),
             captured = std::make_tuple(std::forward<Captures_>(captures)...)](
                    auto&&... args) {
                auto tuple = std::tuple_cat(
                        std::move(captured),
                        std::forward_as_tuple(std::forward<decltype(args)>(args)...));

                if (std::is_same_v<void, decltype(std::apply(fn, tuple))>)
                    std::apply(fn, tuple);
                else
                    return std::apply(fn, tuple);
            };
}

/**
 * Bind front callable with weak reference. Actual callable will be invoked
 *  only when given weak reference is valid.
 */
template <class Callable_, typename Ptr_, typename... Captures_>
auto bind_front_weak(Ptr_&& ref, Callable_&& callable, Captures_&&... captures)
{
    return
            [wptr     = std::weak_ptr{std::forward<Ptr_>(ref)},
             fn       = std::forward<Callable_>(callable),
             captured = std::make_tuple(std::forward<Captures_>(captures)...)](
                    auto&&... args) {
                auto tuple = std::tuple_cat(
                        std::move(captured),
                        std::forward_as_tuple(std::forward<decltype(args)>(args)...));

                if (auto anchor = wptr.lock(); anchor)
                {
                    if (std::is_same_v<void, decltype(std::apply(fn, tuple))>)
                        std::apply(fn, tuple);
                    else
                        return std::apply(fn, tuple);
                }
                else
                {
                    if (std::is_same_v<void, decltype(std::apply(fn, tuple))>)
                        ;
                    else
                        return decltype(std::apply(fn, tuple))();
                }
            };
}
}  // namespace CPPHEADERS_NS_
