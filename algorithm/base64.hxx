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
#include <cstdint>
#include <string_view>

#include "../array_view.hxx"

// assert always
#include "../__namespace__"

namespace CPPHEADERS_NS_::base64 {
namespace detail {
constexpr std::string_view tb_encode
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
          "abcdefghijklmnopqrstuvwxyz"
          "0123456789+/";
static_assert(tb_encode.size() == 64);

constexpr uint8_t tb_decode[256] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff,
        0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
        0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
        0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
        0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

using bytes_t = std::array<char, 3>;
using frame_t = std::array<char, 4>;
static_assert(sizeof(bytes_t) == 3);

constexpr frame_t _encode_single_frame(uint32_t b) noexcept
{
    b = ((b & 0x0000ff) << 16) | (b & 0x00ff00) | ((b & 0xff0000) >> 16);

    return {(tb_encode[(b >> 18) & 0b111111]),
            (tb_encode[(b >> 12) & 0b111111]),
            (tb_encode[(b >> 6) & 0b111111]),
            (tb_encode[(b >> 0) & 0b111111])};
}

constexpr frame_t encode_single_frame(bytes_t const* bytes) noexcept
{
    auto b = *(uint32_t const*)bytes & 0x00'ffffff;
    return _encode_single_frame(b);
}

enum class bytes_length
{
    _1 = 1,
    _2 = 2
};

constexpr frame_t encode_single_frame_with_padding(char const* bytes, bytes_length len) noexcept
{
    auto b = *(uint32_t const*)bytes & (len == bytes_length::_1 ? 0x000000'ff : 0x0000'ffff);
    auto d = _encode_single_frame(b);

    if (len == bytes_length::_1)
        d[2] = d[3] = '=';
    else
        d[3] = '=';

    return d;
}

constexpr uint32_t _decode_single(frame_t frame) noexcept
{
    uint32_t b     = 0;
    bool has_error = false;

    for (int i = 0; i < 4; ++i)
    {
        b += int(tb_decode[frame[i]]) << ((3 - i) * 6);
        has_error |= tb_decode[frame[i]] >= uint8_t{128};
    }

    b = ((b & 0x0000ff) << 16) | (b & 0x00ff00) | ((b & 0xff0000) >> 16);

    return has_error ? ~uint32_t{} : b;
}

constexpr bool decode_single_frame(frame_t frame, bytes_t* bytes) noexcept
{
    auto value  = _decode_single(frame);
    (*bytes)[0] = ((value >> 0) & 0xff);
    (*bytes)[1] = ((value >> 8) & 0xff);
    (*bytes)[2] = ((value >> 16) & 0xff);
    return value == ~uint32_t{};
}

constexpr int decode_last_frame(frame_t frame, bytes_t* bytes) noexcept
{
    auto value   = _decode_single(frame);
    auto success = value != ~uint32_t{};
    auto len     = (3 - (frame[3] == '=') - (frame[2] == '='));

    for (size_t i = 0; i < len; ++i)
    {
        (*bytes)[i] = ((value >> (i * 8)) & 0xff);
    }

    return success * len;
}
}  // namespace detail

constexpr size_t encoded_size(size_t data_length) noexcept
{
    return (data_length + 2) / 3 * 4;
}

template <typename String_>
constexpr size_t decoded_size(String_ const& data) noexcept
{
    static_assert(sizeof(std::declval<String_>()[0]) == 1);
    return (std::size(data) + 3) / 4 * 3
         - (std::end(data)[-1] == '=')
         - (std::end(data)[-2] == '=');
}

template <typename OutIt_>
constexpr void encode_bytes(void const* data, size_t len, OutIt_ out) noexcept
{
    auto src = (detail::bytes_t const*)data;

    for (; len >= 3; len -= 3, ++src)
    {
        auto encoded = detail::encode_single_frame(src);
        *(out++)     = encoded[0];
        *(out++)     = encoded[1];
        *(out++)     = encoded[2];
        *(out++)     = encoded[3];
    }

    if (len > 0)
    {
        auto encoded = detail::encode_single_frame_with_padding(
                src->data(), (detail::bytes_length)len);

        *(out++) = encoded[0];
        *(out++) = encoded[1];
        *(out++) = encoded[2];
        *(out++) = encoded[3];
    }
}

template <typename Ty_, typename OutIt_>
constexpr void encode_one(Ty_ const& data, OutIt_ out) noexcept
{
    encode_bytes(&data, sizeof data, out);
}

template <typename Array_, typename OutIt_>
constexpr void encode(Array_&& array, OutIt_ out) noexcept
{
    constexpr auto elem_size = sizeof(std::declval<Array_>()[0]);
    encode_bytes(std::data(array), elem_size * std::size(array), out);
}

template <typename OutIt_>
constexpr bool decode_bytes(char const* data, size_t size, OutIt_ out)
{
    static_assert(sizeof(*std::declval<OutIt_>()) == 1);

    if ((size & 0b11) != 0)
        throw std::logic_error("data size must be multiplicand of 4!");

    detail::bytes_t decoded{};
    auto it    = (detail::frame_t const*)data,
         end_1 = (detail::frame_t const*)(data + size) - 1;

    bool has_error = false;

    for (; it < end_1; ++it)
    {
        has_error |= detail::decode_single_frame(*it, &decoded);
        *(out++) = decoded[0];
        *(out++) = decoded[1];
        *(out++) = decoded[2];
    }

    auto len = detail::decode_last_frame(*it, &decoded);
    has_error |= len == 0;

    for (auto i = 0; i < len; ++i)
        *(out++) = decoded[i];

    return not has_error;
}

template <typename String_, typename OutIt_>
constexpr bool decode(String_ const& array, OutIt_ out)
{
    constexpr auto elem_size = sizeof(std::declval<String_>()[0]);
    return decode_bytes((char const*)std::data(array), std::size(array) * elem_size, out);
}

}  // namespace CPPHEADERS_NS_::base64