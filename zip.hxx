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
#include "template_utils.hxx"

//
#include "__namespace__"

namespace CPPHEADERS_NS_ {
namespace _zip_impl {
template <typename Arg_>
auto _can_de_deref(int) -> decltype(
        **std::declval<Arg_>(),
        std::integral_constant<
                bool,
                std::is_pointer_v<std::remove_reference_t<decltype(*Arg_{})>>>{});

template <typename>
auto _can_de_deref(...) -> std::false_type;

template <typename Arg_>
auto& _deref_arg(Arg_&& s)
{
    if constexpr (decltype(_can_de_deref<Arg_>(0))::value) {
        return *s;
    } else {
        return s;
    }
}

template <typename... Args_>
class _zip_iterator
{
   public:
    using tuple_type        = std::tuple<Args_...>;
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = std::tuple<std::remove_reference_t<decltype(*Args_{})>...>;
    using difference_type   = ptrdiff_t;
    using reference         = std::tuple<decltype(*std::declval<Args_>())...>;
    using pointer           = value_type*;

   private:
    template <size_t... N_>
    auto _deref(std::index_sequence<N_...>) const
    {
        return std::forward_as_tuple(_deref_arg(std::get<N_>(pack_))...);
    }

    template <size_t N_ = 0>
    bool _compare_strict(_zip_iterator const& op, bool previous) const
    {
        if constexpr (N_ < sizeof...(Args_)) {
            auto result = std::get<N_>(op.pack_) == std::get<N_>(pack_);
            if (result != previous) {
                throw std::invalid_argument("packed tuples has difference lengths");
            }

            return _compare_strict<N_ + 1>(op, result);
        } else {
            return previous;
        }
    }

   public:
    bool operator==(_zip_iterator const& op) const
    {
        return _compare_strict(op, std::get<0>(op.pack_) == std::get<0>(pack_));
    }
    bool operator!=(_zip_iterator const& op) const { return !(*this == op); }

    _zip_iterator& operator++()
    {
        return std::apply([](auto&&... arg) { (++arg, ...); },
                          pack_),
             *this;
    }

    _zip_iterator operator++(int)
    {
        auto copy = *this;
        return ++*this, copy;
    }

    _zip_iterator& operator--()
    {
        return std::apply([](auto&&... arg) { (--arg, ...); },
                          pack_),
             *this;
    }

    _zip_iterator operator--(int)
    {
        auto copy = *this;
        return --*this, copy;
    }

    auto operator*() const
    {
        return _deref(std::make_index_sequence<sizeof...(Args_)>{});
    }

    _zip_iterator& operator+=(difference_type n)
    {
        return std::apply([n](auto&&... arg) { ((arg += n), ...); },
                          pack_),
             *this;
    }
    _zip_iterator& operator-=(difference_type n)
    {
        return std::apply([n](auto&&... arg) { ((arg -= n), ...); },
                          pack_),
             *this;
    }

    friend _zip_iterator operator+(_zip_iterator c, difference_type n) { return c += n; }
    friend _zip_iterator operator-(_zip_iterator c, difference_type n) { return c -= n; }

    friend _zip_iterator operator+(difference_type n, _zip_iterator c) { return c + n; }
    friend _zip_iterator operator-(difference_type n, _zip_iterator c) { return c - n; }

    difference_type operator-(_zip_iterator o) const { return std::get<0>(pack_) - std::get<0>(o.pack_); }

    bool operator<(_zip_iterator o) const { return std::get<0>(pack_) < std::get<0>(o.pack_); }
    bool operator>(_zip_iterator o) const { return o < *this; }

    reference operator[](difference_type n) const { return *(*this + n); }

   public:
    tuple_type pack_;
};

template <typename... Args_>
class _zip_range
{
   public:
    using tuple_type     = std::tuple<Args_...>;
    using iterator       = _zip_iterator<Args_...>;
    using const_iterator = _zip_iterator<Args_...>;

    _zip_iterator<Args_...> begin() const { return {begin_}; }
    _zip_iterator<Args_...> end() const { return {end_}; }

   public:
    tuple_type begin_;
    tuple_type end_;
};

template <typename Ty_, typename... Ph_>
decltype(auto) _container_size(Ty_&& container, Ph_...)
{
    return std::size(container);
}

}  // namespace _zip_impl

template <typename Ty_>
decltype(auto) il(std::initializer_list<Ty_> v)
{
    return v;
}

inline namespace zipper {
/**
 * bind multiple iterable containers with single iterator
 */
template <typename... Containers_>
decltype(auto) zip(Containers_&&... containers)
{
    auto begin = std::make_tuple(std::begin(containers)...);
    auto end   = std::make_tuple(std::end(containers)...);

    _zip_impl::_zip_range<decltype(std::begin(containers))...> zips;
    zips.begin_ = begin;
    zips.end_   = end;
    return zips;
}
}  // namespace zipper
}  // namespace CPPHEADERS_NS_

namespace CPPHEADERS_NS_::utilities {
using namespace CPPHEADERS_NS_::zipper;
}

// tuple overload to receive swap ...
namespace std {
template <typename... Args_,
          typename = std::enable_if_t<(is_reference_v<Args_> && ...)>>
void swap(std::tuple<Args_...> const& a, std::tuple<Args_...> const& b)  //
{
    using tup_t = std::tuple<Args_...>;
    ((tup_t&)a).swap((tup_t&)b);
}
}  // namespace std
