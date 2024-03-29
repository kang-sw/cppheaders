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
#include <algorithm>
#include <limits>

// clang-format off
namespace std{inline namespace literals {}}
// clang-format on

namespace cpph {
inline namespace stdfwd {
using namespace std::literals;

using std::initializer_list;
using std::nothrow;
using std::nullptr_t;

using std::exchange;
using std::invoke;
using std::swap;

using std::clamp;
using std::max;
using std::min;
using std::minmax;

using std::as_const;
using std::forward;
using std::get;
using std::move;

using std::ref;
using std::reference_wrapper;

using std::distance;
using std::size;

using std::decay_t;
using std::remove_const_t;
using std::remove_pointer_t;

using std::enable_if;
using std::enable_if_t;

using std::conditional_t;
using std::integral_constant;

using std::is_enum_v;
using std::is_integral_v;
using std::is_null_pointer_v;

using std::is_base_of_v;
using std::is_convertible_v;

using std::is_const_v;
using std::is_invocable_r_v;
using std::is_invocable_v;
using std::is_same_v;
using std::is_void_v;

using std::declval;
}  // namespace stdfwd

using std::numeric_limits;
}  // namespace cpph

namespace cpph {
// clang-format off
struct empty_struct {};
class empty_class {};
// clang-format on

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
    if constexpr (std::is_invocable_v<FuncT, decltype(std::get<I>(t)), size_t>) {
        f(std::get<I>(t), I);
    } else {
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
    if constexpr (std::is_invocable_v<FuncT, decltype(std::get<I>(t)), size_t>) {
        f(std::get<I>(t), I);
    } else {
        f(std::get<I>(t));
    }
    tuple_for_each<I + 1, FuncT&&, Tp...>(t, std::forward<FuncT>(f));
}

/**
 * wrapper for borrowed ranges
 */
template <typename Begin_, typename End_>
struct _borrowed_range {
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

/**
 * Retrieve function signature's parameter list as tuple.
 * \see https://stackoverflow.com/questions/22630832/get-argument-type-of-template-callable-object
 */

// as seen on http://functionalcpp.wordpress.com/2013/08/05/function-traits/
template <class F>
struct function_traits;

// function pointer
template <class R, class... Args>
struct function_traits<R (*)(Args...)> : public function_traits<R(Args...)> {
};

template <class R, class... Args>
struct function_traits<R(Args...)> {
    using return_type = R;

    static constexpr std::size_t arity = sizeof...(Args);

    template <std::size_t N>
    struct argument {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };
};

// member function pointer
template <class C, class R, class... Args>
struct function_traits<R (C::*)(Args...)> : public function_traits<R(C&, Args...)> {
};

// const member function pointer
template <class C, class R, class... Args>
struct function_traits<R (C::*)(Args...) const> : public function_traits<R(C&, Args...)> {
};

// member object pointer
template <class C, class R>
struct function_traits<R(C::*)> : public function_traits<R(C&)> {
};

// functor
template <class F>
struct function_traits {
   private:
    using call_type = function_traits<decltype(&F::operator())>;

   public:
    using return_type = typename call_type::return_type;

    static constexpr std::size_t arity = call_type::arity - 1;

    template <std::size_t N>
    struct argument {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename call_type::template argument<N + 1>::type;
    };
};

template <class F>
struct function_traits<F&> : public function_traits<F> {
};

template <class F>
struct function_traits<F&&> : public function_traits<F> {
};

template <typename Ptr1_, typename Ptr2_>
bool ptr_equals(Ptr1_&& lhs, Ptr2_&& rhs)
{
    return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}

// Make forwarded type
template <class T>
using as_reference_t = conditional_t<std::is_reference_v<T>, T, T const&>;

// check if has less
template <class Opr_, class X_, class Y_ = X_>
struct has_binary_op {
    template <typename Op_, typename LTy_, typename RTy_>
    static auto _test(int) -> decltype(std::declval<Op_>()(std::declval<LTy_>(), std::declval<RTy_>()), std::true_type{})
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

// tuple utils
template <typename Tuple_>
auto make_forwarded_tuple(Tuple_&& tup)
{
    return std::apply(
            [](auto&&... args) {
                return std::forward_as_tuple(std::forward<decltype(args)>(args)...);
            },
            std::forward<Tuple_>(tup));
}

// enable if
template <bool Test_, typename Ty_ = void>
constexpr auto enable_if_v() -> decltype(std::declval<std::enable_if_t<Test_, Ty_>>());

// constexpr declval
template <typename Ty_>
constexpr Ty_ declval_c() noexcept;

// is_any_of template
template <typename Ty_, typename... Args_>
constexpr bool is_any_of_v = ((std::is_same_v<Ty_, Args_>) || ...);

// is_specialization
// @see https://stackoverflow.com/questions/16337610/how-to-know-if-a-type-is-a-specialization-of-stdvector
template <typename Test, template <typename...> class Ref>
struct is_template_instance_of : std::false_type {
};

template <template <typename...> class Ref, typename... Args>
struct is_template_instance_of<Ref<Args...>, Ref> : std::true_type {
};

template <typename Ty_>
using remove_cvr_t = std::remove_cv_t<std::remove_reference_t<Ty_>>;

template <typename Type_, size_t N = sizeof(Type_)>
class test_size_by_error;

template <typename Ty_>
struct static_const {
    static constexpr Ty_ value{};
};

template <typename Ty_>
constexpr Ty_ static_const<Ty_>::value;

template <typename Ty_>
Ty_& declref() noexcept
{
    return *(Ty_*)nullptr;
}

/// y_combinator for lambda recurse
/// @see https://stackoverflow.com/a/40873657/12923403
template <class F>
struct y_combinator {
    F f;  // the lambda will be stored here

    // a forwarding operator():
    template <class... Args>
    decltype(auto) operator()(Args&&... args) const&
    {
        // we pass ourselves to f, then the arguments.
        return f(*this, std::forward<Args>(args)...);
    }

    // a forwarding operator():
    template <class... Args>
    decltype(auto) operator()(Args&&... args) &
    {
        // we pass ourselves to f, then the arguments.
        return f(*this, std::forward<Args>(args)...);
    }

    // a forwarding operator():
    template <class... Args>
    decltype(auto) operator()(Args&&... args) &&
    {
        // we pass ourselves to f, then the arguments.
        return f(move(*this), std::forward<Args>(args)...);
    }
};

// Deduction guide
template <class F>
y_combinator(F) -> y_combinator<F>;

// helper function that deduces the type of the lambda:
template <class F>
y_combinator<std::decay_t<F>> make_y_combinator(F&& f)
{
    return {std::forward<F>(f)};
}

template <typename T>
using add_reference_t = std::conditional_t<std::is_reference_v<T>, T, T const&>;

// @see https://en.cppreference.com/w/cpp/utility/variant/visit
// helper type for the visitor #4
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// Asserts non-null output parameter
template <class Ptr, class = decltype((void)*declval<Ptr>())>
class non_null
{
   public:
    using pointer = Ptr;
    using value_type = std::remove_reference_t<decltype(*declval<Ptr>())>;

   private:
    Ptr const ref_;

   public:
    template <typename P, class = enable_if_t<is_convertible_v<P, Ptr>>>
    non_null(P&& ptr) noexcept : ref_(std::forward<P>(ptr))
    {
        if (ref_ == Ptr{}) { abort(); }
    }

    non_null() noexcept = delete;
    non_null(nullptr_t) noexcept = delete;
    non_null& operator=(nullptr_t) noexcept = delete;

    non_null(const non_null&) noexcept = default;
    non_null(non_null&&) noexcept = default;
    non_null& operator=(const non_null&) noexcept = default;
    non_null& operator=(non_null&&) noexcept = default;

    pointer get() const noexcept { return ref_; }

   public:
    auto& operator->() const noexcept { return ref_; }
    auto& operator*() const noexcept { return *ref_; }
};

// Alias for output
template <class T>
using inout = non_null<T*>;

template <class T>
using out = non_null<T*>;

}  // namespace cpph
