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

//
// Created by Seungwoo on 2021-08-26.
//
#pragma once
#include <cassert>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>

//

namespace cpph {
template <typename Ty_>
constexpr bool _is_buffer_elem_v
        = (sizeof(Ty_) == 1) && std::is_trivial_v<Ty_>;

template <typename Array_>
class _array_reinterpret_accessor
{
   public:
    template <typename RTy_>
    auto& as(size_t offset = 0) const
    {
        using value_type = typename Array_::value_type;
        enum {
            is_const = std::is_const_v<value_type>
        };

        using rtype = std::conditional_t<is_const, std::add_const_t<RTy_>, RTy_>&;
        auto buf = &((Array_*)this)->at(offset);

        // verify
        (void)((Array_*)this)->at(offset + sizeof(RTy_) - 1);

        return reinterpret_cast<rtype>(*buf);
    }
};

struct _empty_base {};
template <typename Array_, typename Ty_>
using _array_view_base = std::conditional_t<_is_buffer_elem_v<Ty_>,
                                            _array_reinterpret_accessor<Array_>,
                                            _empty_base>;

template <typename Ty_>
class array_view : public _array_view_base<array_view<Ty_>, Ty_>
{
   public:
    using value_type = Ty_;
    using pointer = value_type*;
    using const_pointer = value_type const*;
    using reference = value_type&;

   public:
    constexpr array_view() noexcept = default;
    constexpr array_view(Ty_* p, size_t n) noexcept
            : _ptr(p), _size(n) {}

    template <typename Range_>
    constexpr array_view(Range_&& p) noexcept
            : array_view(std::data(p), std::size(p))
    {
    }

    template <size_t N_>
    constexpr array_view(Ty_ (&p)[N_]) noexcept
            : array_view(p, N_)
    {
    }

    constexpr auto size() const noexcept { return _size; }
    constexpr auto data() const noexcept { return _ptr; }
    constexpr auto data() noexcept { return _ptr; }

    constexpr auto begin() noexcept { return _ptr; }
    constexpr auto begin() const noexcept { return _ptr; }
    constexpr auto end() noexcept { return _ptr + _size; }
    constexpr auto end() const noexcept { return _ptr + _size; }

    constexpr auto& front() const noexcept { return at(0); }
    constexpr auto& back() const noexcept { return at(_size - 1); }

    constexpr auto empty() const noexcept { return size() == 0; }

    constexpr auto subspan(size_t offset, size_t n = ~size_t{}) const
    {
        if (offset >= _size) { return array_view{_ptr, 0}; }
        return array_view{_ptr + offset, std::min(n, _size - offset)};
    }

    template <typename RTy_>
    constexpr bool operator==(RTy_&& op) const noexcept
    {
        return std::equal(begin(), end(), std::begin(op), std::end(op));
    }

    template <typename RTy_>
    constexpr bool operator!=(RTy_&& op) const noexcept
    {
        return !(*this == std::forward<RTy_>(op));
    }

    template <typename RTy_>
    constexpr bool operator<(RTy_ && op) const noexcept
    {
        return std::lexicographical_compare(begin(), end(), std::begin(op), std::end(op));
    }

    constexpr auto& operator[](size_t idx)
    {
#ifndef NDEBUG
        _verify_idx(idx);
#endif
        return _ptr[idx];
    };

    constexpr auto& operator[](size_t idx) const
    {
#ifndef NDEBUG
        _verify_idx(idx);
#endif
        return _ptr[idx];
    };

    constexpr auto& at(size_t idx) { return _verify_idx(idx), _ptr[idx]; }
    constexpr auto& at(size_t idx) const { return _verify_idx(idx), _ptr[idx]; }

   private:
    constexpr void _verify_idx(size_t idx) const
    {
        if (idx >= _size) { throw std::out_of_range{"bad index"}; }
    }

   private:
    Ty_* _ptr;
    size_t _size;
};

template <typename Ty_>
array_view(Ty_*, size_t n) -> array_view<Ty_>;

template <typename Range_, typename = decltype(std::size(std::declval<Range_>()))>
array_view(Range_&&) -> array_view<std::decay_t<decltype(*std::data(std::declval<Range_>()))>>;

template <typename Ty_>
using const_array_view = array_view<Ty_ const>;

template <typename Ty_>
using mutable_array_view = array_view<Ty_>;

template <typename Ty_>
constexpr bool is_binary_compatible_v
        = (std::is_trivially_destructible_v<Ty_>)&&(std::is_trivially_copyable_v<Ty_>);

template <>
class array_view<void> : public array_view<char>
{
   public:
    array_view() noexcept = default;

    template <typename Container_,
              typename = std::enable_if_t<is_binary_compatible_v<
                      typename std::remove_reference_t<Container_>::value_type>>>
    explicit array_view(Container_&& other) noexcept
            : array_view<char>(reinterpret_cast<char*>(std::data(other)),
                               std::size(other) * sizeof(*std::data(other)))
    {
    }

    template <typename Ty_, typename = std::enable_if_t<is_binary_compatible_v<Ty_>>>
    array_view(Ty_* data, size_t size) noexcept
            : array_view<char>(reinterpret_cast<char*>(data),
                               sizeof(Ty_) * size)
    {
    }

    template <typename Container_,
              typename = std::enable_if_t<is_binary_compatible_v<
                      typename std::remove_reference_t<Container_>::value_type>>>
    array_view& operator=(Container_&& other) noexcept
    {
        array_view<char>::operator=(
                array_view<char>(
                        reinterpret_cast<char*>(std::data(other)),
                        std::size(other) * sizeof(*std::data(other))));
        return *this;
    }
};

template <>
class array_view<void const> : public array_view<char const>
{
   public:
    array_view() noexcept = default;

    template <typename Container_,
              typename = std::enable_if_t<is_binary_compatible_v<
                      typename std::remove_reference_t<Container_>::value_type>>>
    explicit array_view(Container_&& other) noexcept
            : array_view<char const>(reinterpret_cast<char const*>(std::data(other)),
                                     std::size(other) * sizeof(*std::data(other)))
    {
    }

    template <typename Ty_,
              typename = std::enable_if_t<is_binary_compatible_v<Ty_>>>
    array_view(Ty_ const* data, size_t size) noexcept
            : array_view<char const>(reinterpret_cast<char const*>(data),
                                     sizeof(Ty_) * size)
    {
    }

    template <typename Container_,
              typename = std::enable_if_t<is_binary_compatible_v<
                      typename std::remove_reference_t<Container_>::value_type>>>
    array_view& operator=(Container_&& other) noexcept

    {
        array_view<char const>::operator=(
                array_view<char const>(
                        reinterpret_cast<char const*>(std::data(other)),
                        std::size(other) * sizeof(*std::data(other))));
        return *this;
    }

    array_view(array_view<void> v)
            : array_view<char const>(v.data(), v.size())
    {
    }

    array_view& operator=(array_view<void> v)
    {
        array_view<char const>::operator=({v.data(), v.size()});
        return *this;
    }
};

using const_buffer_view = array_view<void const>;
using mutable_buffer_view = array_view<void>;

template <typename Range_>
constexpr auto make_view(Range_&& array)
{
    return array_view{std::data(array), std::size(array)};
}

template <typename Range_>
constexpr auto view_array(Range_&& array)
{
    return array_view{std::data(array), std::size(array)};
}

template <typename T>
auto create_temporary_array(void* memory, size_t nelem)
{
    if constexpr (std::is_trivially_destructible_v<T>) {
        return array_view{(T*)memory, nelem};
    } else {
        struct disposer_t {
            size_t elem_size;
            using pointer = void*;
            void operator()(pointer p) const noexcept(std::is_nothrow_destructible_v<T>)
            {
                for (auto i = 0; i < elem_size; ++i) {
                    ((T*)p)[i].~T();
                }
            }
        };

        auto pointer = std::unique_ptr<void, disposer_t>{memory, disposer_t{nelem}};
        auto view = array_view{(T*)memory, nelem};

        return std::make_pair(view, std::move(pointer));
    }
}

}  // namespace cpph

#if __has_include(<range/v3/range/concepts.hpp>)
#    include <range/v3/range/concepts.hpp>

namespace ranges {
template <typename Ty_>
RANGES_INLINE_VAR constexpr bool
        enable_borrowed_range<cpph::array_view<Ty_>> = true;
}
#endif
