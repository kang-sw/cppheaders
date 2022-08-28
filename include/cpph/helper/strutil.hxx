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
#include <cctype>
#include <cstdint>

#include "cpph/utility/templates.hxx"

namespace cpph::strutil {

template <typename Char, typename OutIt>
void _escape_ch(Char ch, OutIt&& out)
{
    if (ch < 0 || std::isprint(ch) && ch != '\\' && ch != '\"')  // && ch != '\?' && ch != '\'' )
        *out = ch;
    else {
        *out++ = '\\';
        switch (ch) {
            case '\a': *out++ = ('a'); break;
            case '\b': *out++ = ('b'); break;
            case '\f': *out++ = ('f'); break;
            case '\n': *out++ = ('n'); break;
            case '\r': *out++ = ('r'); break;
            case '\t': *out++ = ('t'); break;
            case '\v': *out++ = ('v'); break;
            case '\\': *out++ = ('\\'); break;
            case '\'': *out++ = ('\''); break;
            case '\"': *out++ = ('\"'); break;
            case '\?': *out++ = ('\?'); break;
            default: {
                uint32_t cc;

                if constexpr (sizeof ch == 1)
                    cc = uint8_t(ch);
                else if constexpr (sizeof ch == 2)
                    cc = uint16_t(ch);
                else
                    cc = uint32_t(ch);

                constexpr auto table = "0123456789abcdef";
                //                if (cc < 256) {
                //                    *out++ = 'x';
                //
                //                    *out++ = table[(cc & 0xf0) >> 4];
                //                    *out++ = table[(cc & 0x0f) >> 0];
                //                } else
                if (cc < 65535) {
                    *out++ = 'u';

                    *out++ = table[(cc & 0xf000) >> 12];
                    *out++ = table[(cc & 0x0f00) >> 8];
                    *out++ = table[(cc & 0x00f0) >> 4];
                    *out++ = table[(cc & 0x000f) >> 0];
                } else {
                    throw;  // error
                }
            }
        }
    }
}

template <typename InIt, typename OutIt>
void json_escape(InIt begin, InIt end, OutIt out)
{
    for (auto it = begin; it != end; ++it) { _escape_ch(*it, out); }
}

inline int hexval(char hi, char lo)
{
    hi = (hi >= 'a') ? (hi - 'a' + 0xa) : (hi - '0');
    lo = (lo >= 'a') ? (lo - 'a' + 0xa) : (lo - '0');

    return (hi << 4) + lo;
}

template <typename InputIt, typename OutIt>
void json_unescape(InputIt begin, InputIt end, OutIt out)
{
    for (auto it = begin; it != end;) {
        char ch = *it++;
        if (ch != '\\') {
            *out++ = ch;
        } else {
            if (it == end) { return; }
            switch (ch = *it++) {
                case 'a': *out++ = '\a'; break;
                case 'b': *out++ = '\b'; break;
                case 'f': *out++ = '\f'; break;
                case 'n': *out++ = '\n'; break;
                case 'r': *out++ = '\r'; break;
                case 't': *out++ = '\t'; break;
                case 'v': *out++ = '\v'; break;
                case '\\': *out++ = '\\'; break;
                case '\'': *out++ = '\''; break;
                case '"': *out++ = '\"'; break;
                case '?': *out++ = '\?'; break;

                case 'u': {
                    if (it == end) { return; }
                    char hi = *it++;
                    if (it == end) { return; }
                    char lo = *it++;

                    *out++ = hexval(hi, lo);
                    [[fallthrough]];
                }

                case 'x': {
                    if (it == end) { return; }
                    char hi = *it++;
                    if (it == end) { return; }
                    char lo = *it++;

                    *out++ = hexval(hi, lo);
                    break;
                }
            }
        }
    }
}

/**
 * Validate UTF-8 string by leaping its
 *
 * @see https://gist.github.com/ichramm/3ffeaf7ba4f24853e9ecaf176da84566
 */
template <class BeginIt, class EndIt, class OutIt,
          class = enable_if_t<sizeof(decay_t<decltype(*declval<BeginIt>())>) == 1>>
size_t validate_utf8(BeginIt it, EndIt const& end, OutIt out)
{
    // Based on this one: http://www.zedwood.com/article/cpp-is-valid-utf8-string-function
    unsigned char shelf[4];
    size_t new_size = 0;
    int n;
    unsigned char c;

    for (; it != end;) {
        c = (unsigned char)*it++;

        if (0x00 <= c && c <= 0x7f) {
            *out++ = c, ++new_size;
            continue;
        } else if ((c & 0xE0) == 0xC0) {
            n = 1;
        } else if ((c & 0xF0) == 0xE0) {
            n = 2;  // 1110bbbb
        } else if ((c & 0xF8) == 0xF0) {
            n = 3;  // 11110bbb
        } else {
            continue;
        }

        // first character
        shelf[0] = c;

        for (auto j = 0; j < n; ++j) {
            if (it == end) {
                goto END;
            }

            if (((shelf[j + 1] = (unsigned char)*it++) & 0xC0) != 0x80) {
                goto SKIP;
            }
        }

        for (auto j = 0; j < n + 1; ++j) {
            *out++ = shelf[j];
        }

        new_size += n + 1;
    SKIP:;
    }

END:;
    return new_size;
}
}  // namespace cpph::strutil
