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
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "type_traits.hxx"

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_ {
namespace hasher {
constexpr static uint64_t FNV_PRIME       = 0x100000001b3ull;
constexpr static uint64_t FNV_OFFSET_BASE = 0xcbf29ce484222325ull;

/// hash a single byte
constexpr inline uint64_t fnv1a_byte(unsigned char byte, uint64_t hash)
{
    return (hash ^ byte) * FNV_PRIME;
}

template <typename Ty_>
constexpr inline uint64_t fnv1a_64(Ty_ const& val, uint64_t base = FNV_OFFSET_BASE);

template <typename It_>
constexpr inline uint64_t fnv1a_64(It_ begin, It_&& end, uint64_t base = FNV_OFFSET_BASE)
{
    for (; begin != end; ++begin)
    {
        if constexpr (sizeof(*begin) == 1)
            base = fnv1a_byte(*begin, base);
        else
            base = fnv1a_64(*begin, base);
    }
    return base;
}

template <typename T>
using _is_range_t = decltype(std::begin(std::declval<T>()));

template <typename Ty_>
constexpr inline uint64_t fnv1a_64(Ty_ const& val, uint64_t base)
{
    if constexpr (type_traits::is_detected_v<_is_range_t, Ty_>)
    {
        return fnv1a_64(std::begin(val), std::end(val), base);
    }
    else if constexpr (std::is_trivial_v<Ty_>)
    {
        return fnv1a_64((char const*)(&val), (char const*)(&val + 1), base);
    }
    else
    {
        val.GENERATE_STATIC_ASSERT();
    }
}
}  // namespace hasher

template <typename Label_>
struct basic_key
{
    bool operator<(basic_key const& other) const noexcept { return value < other.value; }
    bool operator==(basic_key const& other) const noexcept { return value == other.value; }

    template <typename Other_,
              typename = std::enable_if_t<std::is_arithmetic_v<Other_>>>
    friend bool operator<(Other_&& v, basic_key const& self) noexcept
    {
        return v < self.value;
    }

    template <typename Other_,
              typename = std::enable_if_t<std::is_arithmetic_v<Other_>>>
    friend bool operator<(basic_key const& self, Other_&& v) noexcept
    {
        return self.value < v;
    }

    explicit operator bool() const noexcept { return valid(); }
    bool valid() const noexcept { return value != 0; }

    template <typename Ty_>
    [[deprecated("Use hash() instead")]]  //
    constexpr static basic_key
    create(Ty_&& r) noexcept
    {
        return {hasher::fnv1a_64(r)};
    }

    template <typename Ty_>
    constexpr static basic_key hash(Ty_&& r) noexcept
    {
        return {hasher::fnv1a_64(r)};
    }

   public:
    uint64_t value = {};
};

}  // namespace CPPHEADERS_NS_

namespace std {
template <typename Label_>
struct hash<CPPHEADERS_NS_::basic_key<Label_>>
{
    auto operator()(CPPHEADERS_NS_::basic_key<Label_> const& s) const noexcept
    {
        return ::std::hash<uint64_t>{}(s.value);
    }
};
}  // namespace std
