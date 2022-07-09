
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

#include "../detail/object_core.hxx"
#include "cpph/container/buffer.hxx"

/*
 * Binary
 */
namespace cpph {
template <typename, class = void>
constexpr bool _has_data_fn = false;
template <typename T>
constexpr bool _has_data_fn<
        T,
        std::void_t<decltype(std::data(std::declval<T>()))>> = true;

template <typename Container_, class = void>
class binary;

template <typename Container_>
class binary<
        Container_,
        enable_if_t<
                (is_binary_compatible_v<typename Container_::value_type>)  //
                &&(not _has_data_fn<Container_>)>>
        : public Container_
{
   public:
    Container_& ref() noexcept { return *this; }
    Container_ const& ref() const noexcept { return *this; }
    Container_&& move() && noexcept { return std::move(*this); }
    void swap(Container_& other) noexcept { std::swap(*this, other); }

   public:
    using Container_::Container_;

    enum {
        is_container = true,
        is_contiguous = false
    };
};

template <typename Container_>
class binary<Container_,
             enable_if_t<is_binary_compatible_v<decay_t<decltype(*std::data(std::declval<Container_>()))>>>>
        : public Container_
{
   public:
    Container_& ref() noexcept { return *this; }
    Container_ const& ref() const noexcept { return *this; }
    Container_&& move() && noexcept { return std::move(*this); }
    void swap(Container_& other) noexcept { std::swap(*this, other); }

   public:
    using Container_::Container_;

    enum {
        is_container = true,
        is_contiguous = true
    };
};

template <typename ValTy_>
class binary<ValTy_, enable_if_t<is_binary_compatible_v<ValTy_> && not _has_data_fn<ValTy_>>> : ValTy_
{
   public:
    using ValTy_::ValTy_;

    auto self() const noexcept { return (ValTy_ const*)this; }
    auto self() noexcept { return (ValTy_*)this; }

    enum {
        is_container = false,
    };
};

template <typename, typename = void>
struct chunk;

template <typename T>
struct chunk<T, enable_if_t<std::is_trivial_v<T> && not std::is_fundamental_v<T>>> : public T {
   public:
    using T::T;
};

template <typename T>
struct chunk<T, enable_if_t<std::is_trivial_v<T> && std::is_fundamental_v<T>>> {
    T value;
};

}  // namespace cpph