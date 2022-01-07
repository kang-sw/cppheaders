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
#include <memory>
#include <tuple>

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_ {
/**
 * @see https://stackoverflow.com/questions/1198260/how-can-you-iterate-over-the-elements-of-an-stdtuple
 * Iterate over tempalte
 */
template <std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
tuple_for_each(std::tuple<Tp...>&, FuncT&&)  // Unused arguments are given no names.
{
}

template <std::size_t I = 0, typename FuncT, typename... Tp>
        inline typename std::enable_if < I<sizeof...(Tp), void>::type
        tuple_for_each(std::tuple<Tp...>& t, FuncT&& f)
{
    if constexpr (std::is_invocable_v<FuncT, decltype(std::get<I>(t)), size_t>)
    {
        f(std::get<I>(t), I);
    }
    else
    {
        f(std::get<I>(t));
    }
    tuple_for_each<I + 1, FuncT&&, Tp...>(t, std::forward<FuncT>(f));
}

template <std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
tuple_for_each(std::tuple<Tp...> const&, FuncT&&)  // Unused arguments are given no names.
{
}

template <std::size_t I = 0, typename FuncT, typename... Tp>
        inline typename std::enable_if < I<sizeof...(Tp), void>::type
        tuple_for_each(std::tuple<Tp...> const& t, FuncT&& f)
{
    if constexpr (std::is_invocable_v<FuncT, decltype(std::get<I>(t)), size_t>)
    {
        f(std::get<I>(t), I);
    }
    else
    {
        f(std::get<I>(t));
    }
    tuple_for_each<I + 1, FuncT&&, Tp...>(t, std::forward<FuncT>(f));
}

/**
 * wrapper for borrowed ranges
 */
template <typename Begin_, typename End_>
struct _borrowed_range
{
    Begin_ _begin;
    End_ _end;

    auto begin() const noexcept { return _begin; }
    auto end() const noexcept { return _end; }

    auto begin() noexcept { return _begin; }
    auto end() noexcept { return _end; }

    bool size() const noexcept { return std::distance(_begin, _end); }
    bool empty() const noexcept { return _begin == _end; }

    auto& front() const noexcept { return *_begin; }
    auto& back() const noexcept { return *(std::prev(_end, 1)); }
};

template <typename Begin_, typename End_>
constexpr auto
make_iterable(Begin_&& begin, End_&& end) -> _borrowed_range<Begin_, End_>
{
    return {std::forward<Begin_>(begin), std::forward<End_>(end)};
}

using shared_null = std::shared_ptr<nullptr_t>;
using weak_null   = std::shared_ptr<nullptr_t>;
inline shared_null make_null() { return std::make_shared<nullptr_t>(); }

/**
 * Retrieve function signature's parameter list as tuple.
 * \see https://stackoverflow.com/questions/22630832/get-argument-type-of-template-callable-object
 */

// as seen on http://functionalcpp.wordpress.com/2013/08/05/function-traits/
template <class F>
struct function_traits;

// function pointer
template <class R, class... Args>
struct function_traits<R (*)(Args...)> : public function_traits<R(Args...)>
{
};

template <class R, class... Args>
struct function_traits<R(Args...)>
{
    using return_type = R;

    static constexpr std::size_t arity = sizeof...(Args);

    template <std::size_t N>
    struct argument
    {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };
};

// member function pointer
template <class C, class R, class... Args>
struct function_traits<R (C::*)(Args...)> : public function_traits<R(C&, Args...)>
{
};

// const member function pointer
template <class C, class R, class... Args>
struct function_traits<R (C::*)(Args...) const> : public function_traits<R(C&, Args...)>
{
};

// member object pointer
template <class C, class R>
struct function_traits<R(C::*)> : public function_traits<R(C&)>
{
};

// functor
template <class F>
struct function_traits
{
   private:
    using call_type = function_traits<decltype(&F::operator())>;

   public:
    using return_type = typename call_type::return_type;

    static constexpr std::size_t arity = call_type::arity - 1;

    template <std::size_t N>
    struct argument
    {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename call_type::template argument<N + 1>::type;
    };
};

template <class F>
struct function_traits<F&> : public function_traits<F>
{
};

template <class F>
struct function_traits<F&&> : public function_traits<F>
{
};

template <typename Ptr1_, typename Ptr2_>
bool ptr_equals(Ptr1_&& lhs, Ptr2_&& rhs)
{
    return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}

// check if has less
template <class Opr_, class X_, class Y_ = X_>
struct has_binary_op
{
    template <typename Op_, typename LTy_, typename RTy_>
    static auto _test(int) -> decltype(
            std::declval<Op_>()(std::declval<LTy_>(), std::declval<RTy_>()), std::true_type{})
    {
        return {};
    }

    template <typename Op_, typename LTy_, typename RTy_>
    static auto _test(...) -> std::false_type
    {
        return {};
    }

    using type = decltype(_test<Opr_, X_, Y_>(0));
};

template <class Opr_, class X_, class Y_ = X_>
constexpr bool has_binary_op_v = has_binary_op<Opr_, X_, Y_>::type::value;

//
template <typename Ty_, typename Label_ = void>
struct singleton
{
    static Ty_& get() noexcept
    {
        static Ty_ instance;
        return instance;
    }
};

}  // namespace CPPHEADERS_NS_