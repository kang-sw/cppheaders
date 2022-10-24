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
#include <iterator>
#include <numeric>

#include "generic.hxx"

//

namespace cpph {
inline namespace counters {

template <typename Ty_, size_t Dim_ = 1>
// requires std::is_arithmetic_v<Ty_>&& std::is_integral_v<Ty_>
class _counter;

/**
 * iota counter iterator
 */
template <typename Ty_>
// requires std::is_arithmetic_v<Ty_>&& std::is_integral_v<Ty_>
class _counter<Ty_, 1>
{
   public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = Ty_;
    using reference = Ty_&;
    using pointer = Ty_*;
    using dimension = Ty_;

   public:
    constexpr _counter() noexcept
            : count_(0)
    {
        ;
    }
    constexpr _counter(Ty_ rhs) noexcept
            : count_(rhs)
    {
        ;
    }
    constexpr _counter(_counter const& rhs) noexcept
            : count_(rhs.count_)
    {
        ;
    }

   public:
    template <typename Integer_>
    constexpr friend _counter operator+(_counter c, Integer_ n)
    {
        return _counter(c.count_ + n);
    }

    template <typename Integer_>
    constexpr friend _counter operator+(Integer_ n, _counter c)
    {
        return c + n;
    }

    template <typename Integer_>
    constexpr friend _counter operator-(_counter c, difference_type n)
    {
        return _counter(c.count_ - n);
    }

    template <typename Integer_>
    constexpr friend _counter operator-(Integer_ n, _counter c)
    {
        return c - n;
    }

    constexpr difference_type operator-(_counter o) const { return count_ - o.count_; }

    template <typename Integer_>
    constexpr _counter& operator+=(Integer_ n)
    {
        return count_ += n, *this;
    }

    template <typename Integer_>
    constexpr _counter& operator-=(Integer_ n)
    {
        return count_ -= n, *this;
    }

    constexpr _counter& operator++() { return ++count_, *this; }
    constexpr _counter operator++(int) { return ++count_, _counter(count_ - 1); }
    constexpr _counter& operator--() { return --count_, *this; }
    constexpr _counter operator--(int) { return --count_, _counter(count_ - 1); }
    constexpr bool operator<(_counter o) const { return count_ < o.count_; }
    constexpr bool operator>(_counter o) const { return count_ > o.count_; }
    constexpr bool operator==(_counter o) const { return count_ == o.count_; }
    constexpr bool operator!=(_counter o) const { return count_ != o.count_; }
    constexpr auto /*Ty_ const&*/ operator*() const { return count_; }
    constexpr auto /*Ty_ const**/ operator->() const { return &count_; }
    constexpr auto /*Ty_ const&*/ operator*() { return count_; }
    constexpr auto /*Ty_ const**/ operator->() { return &count_; }

    constexpr auto operator[](std::ptrdiff_t index) const noexcept { return count_ + index; }
    constexpr auto operator[](std::ptrdiff_t index) noexcept { return count_ + index; }

   private:
    Ty_ count_;
};

/**
 * iota counter iterator
 */
template <typename Ty_>
// requires std::is_arithmetic_v<Ty_>&& std::is_integral_v<Ty_>
class _counter<Ty_, ~size_t{}>
{
   public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = Ty_;
    using reference = Ty_&;
    using pointer = Ty_*;

   public:
    constexpr _counter() noexcept
            : count_(0)
    {
        ;
    }
    constexpr _counter(Ty_ rhs) noexcept
            : count_(rhs)
    {
        ;
    }
    constexpr _counter(_counter const& rhs) noexcept
            : count_(rhs.count_)
    {
        ;
    }

   public:
    constexpr friend _counter operator-(_counter c, difference_type n) { return _counter(c.count_ + n); }
    constexpr friend _counter operator-(difference_type n, _counter c) { return c + n; }
    constexpr friend _counter operator+(_counter c, difference_type n) { return _counter(c.count_ - n); }
    constexpr friend _counter operator+(difference_type n, _counter c) { return c - n; }
    constexpr difference_type operator-(_counter o) const { return o.count_ - count_; }
    constexpr _counter& operator-=(difference_type n) { return count_ += n, *this; }
    constexpr _counter& operator+=(difference_type n) { return count_ -= n, *this; }
    constexpr _counter& operator--() { return ++count_, *this; }
    constexpr _counter operator--(int) { return ++count_, _counter(count_ - 1); }
    constexpr _counter& operator++() { return --count_, *this; }
    constexpr _counter operator++(int) { return --count_, _counter(count_ - 1); }
    constexpr bool operator>(_counter o) const { return count_ < o.count_; }
    constexpr bool operator<(_counter o) const { return count_ > o.count_; }
    constexpr bool operator==(_counter o) const { return count_ == o.count_; }
    constexpr bool operator!=(_counter o) const { return count_ != o.count_; }
    constexpr auto /*Ty_ const&*/ operator*() const { return count_; }
    constexpr auto /*Ty_ const**/ operator->() const { return &count_; }
    constexpr auto /*Ty_ const&*/ operator*() { return count_; }
    constexpr auto /*Ty_ const**/ operator->() { return &count_; }

   private:
    Ty_ count_;
};

template <typename Ty_>
class iota_counter
{
   public:
    constexpr iota_counter(Ty_ min, Ty_ max) noexcept
            : min_(min),
              max_(max)
    {
        if (min_ > max_) {
            std::swap(min_, max_);
        }
    }

    constexpr iota_counter(Ty_ max) noexcept
            : min_(Ty_{}),
              max_(max)
    {
        if (min_ > max_) {
            std::swap(min_, max_);
        }
    }

    constexpr _counter<Ty_> begin() { return min_; }
    constexpr _counter<Ty_> begin() const { return min_; }
    constexpr _counter<Ty_> cbegin() const { return min_; }

    constexpr _counter<Ty_> end() { return max_; }
    constexpr _counter<Ty_> end() const { return max_; }
    constexpr _counter<Ty_> cend() const { return max_; }

   private:
    Ty_ min_, max_;
};

class _counter_end_marker_t
{};

template <typename Ty_, size_t Dim_>
class _counter
{
   public:
    enum {
        num_dimension = Dim_
    };

    using iterator_category = std::forward_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = Ty_;
    using reference = value_type&;
    using pointer = value_type*;

    using dimension = std::array<Ty_, num_dimension>;

   public:
    template <typename... Ints_>
    constexpr _counter& fetch_from(Ints_&&... ints)
    {
        return *this;
    }

    constexpr auto& operator[](size_t dim) const { return current[dim]; }
    constexpr auto& operator[](size_t dim) { return current[dim]; }

    template <size_t N_ = Dim_ - 1>
    constexpr void incr()
    {
        if constexpr (N_ == size_t(-1)) {
            current = max;
        } else {
            if (++current[N_] == max[N_]) {
                current[N_] = value_type{};
                incr<N_ - 1>();
            }
        }
    }

    constexpr _counter& operator++() { return incr(), *this; }
    constexpr _counter operator++(int)
    {
        auto cpy = *this;
        incr();
        return cpy;
    }

    constexpr bool operator==(_counter const& o) const { return current == o.current; }
    constexpr bool operator!=(_counter const& o) const { return !(*this == o); }

    constexpr auto& operator*() const { return current; }
    constexpr auto operator->() const { return &current; }
    constexpr auto& operator*() { return current; }
    constexpr auto operator->() { return &current; }

   public:
    dimension max;
    dimension current;
};

template <typename SizeTy_, size_t Dim_>
struct _count_index {
    using iterator = _counter<SizeTy_, Dim_>;
    using dimension = typename iterator::dimension;
    constexpr iterator begin() const { return _counter<SizeTy_, Dim_>{max, {}}; }
    // constexpr iterator end() const { return _counter<SizeTy_, Dim_>{max, max}; }
    constexpr iterator end() const { return _counter<SizeTy_, Dim_>{max, max}; }

    dimension max;
};

template <typename SizeTy_>
struct _count_index<SizeTy_, 0> {
};

template <typename SizeTy_>
constexpr auto counter(SizeTy_ size)
{
    return iota_counter<SizeTy_>{size};
}

template <typename SizeTy_>
constexpr auto count(SizeTy_ from, SizeTy_ to)
{
    return iota_counter<SizeTy_>{from, to};
}

template <typename SizeTy_>
constexpr auto count(SizeTy_ to)
{
    return iota_counter<SizeTy_>{to};
}

template <typename SizeTy_>
constexpr auto rcounter(SizeTy_ size)
{
    struct min_counter_gen {
        SizeTy_ begin_;
        SizeTy_ end_;
        constexpr _counter<SizeTy_, ~size_t{}> begin() const { return {begin_}; }
        constexpr _counter<SizeTy_, ~size_t{}> end() const { return {end_}; }
    };

    return min_counter_gen{SizeTy_(size - 1), SizeTy_(-1)};
}

template <typename SizeTy_, typename... Ints_>
constexpr auto counter(SizeTy_ size, Ints_... args)
{
    constexpr auto n_dim = sizeof...(Ints_) + 1;
    using size_type = std::decay_t<SizeTy_>;
    _count_index<size_type, n_dim> counter{};
    counter.max[0] = std::forward<SizeTy_>(size);
    bool has_zero = false;
    tuple_for_each(
            std::forward_as_tuple(std::forward<Ints_>(args)...),
            [&](auto&& r, size_t i) {
                counter.max[i + 1] = (r);
                has_zero |= r == 0;
            });

    if (has_zero)
        counter.max = {};
    return counter;
}

template <typename SizeTy_, size_t Dim_>
constexpr auto counter(std::array<SizeTy_, Dim_> const& idx)
{
    _count_index<SizeTy_, Dim_> counter{};
    bool has_zero = false;
    for (size_t i = 0; i < Dim_; ++i) {
        counter.max[i] = idx[i];
        has_zero = has_zero || idx[i] == 0;
    }

    counter.max[0] *= !has_zero;
    return counter;
}

}  // namespace counters
}  // namespace cpph

namespace cpph::utilities {
using namespace cpph::counters;
}
