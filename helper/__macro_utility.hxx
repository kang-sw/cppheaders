/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2021.
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
 ******************************************************************************/

#pragma once
#include <array>
#include <optional>
#include <string_view>

#include "../template_utils.hxx"

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_::archiving::detail {
template <const auto& s>
constexpr size_t _count_words() noexcept
{
    size_t n = 1;
    for (int i = 0; i < std::size(s); ++i)
    {
        auto c = s[i];
        n += c == ',';
    }

    return n;
}

constexpr bool _is_word_char(char c) noexcept
{
    return c >= '0' && c <= '9'
        || c >= 'a' && c <= 'z'
        || c >= 'A' && c <= 'Z'
        || c == '_';
}

template <const auto& s>
constexpr std::pair<size_t, size_t>
_words_boundary(size_t n) noexcept
{
    size_t begin = 0;
    for (int i = 0; i < std::size(s); ++i)
    {
        auto c = s[i];
        if (n == 0)
            break;

        n -= c == ',';
        ++begin;
    }

    for (size_t i = begin; i < std::size(s); ++i)
    {
        auto c = s[i];
        if (_is_word_char(c))
            break;
        else
            ++begin;
    }

    size_t end = begin + 1;
    for (size_t i = end; i < std::size(s); ++i, ++end)
    {
        auto c = s[i];
        if (not _is_word_char(c))
            break;
    }

    return {begin, end};
}

template <const auto& str>
constexpr auto break_VA_ARGS() noexcept
{
    constexpr auto n_words = _count_words<str>();
    auto result            = std::array<std::string_view, n_words>{};

    for (size_t i = 0; i < result.size(); ++i)
    {
        auto [begin, end] = _words_boundary<str>(i);
        result[i]         = std::string_view{str + begin, end - begin};
    }

    return result;
}

template <size_t N_>
auto views_to_strings(std::array<std::string_view, N_> const& views) noexcept
{
    std::array<std::string, N_> out;
    for (size_t i = 0; i < N_; ++i)
        out[i] = views[i];

    return out;
}

template <typename Ty_>
struct _is_optional : std::false_type
{
};

template <typename Ty_>
struct _is_optional<std::optional<Ty_>> : std::true_type
{
};

template <typename Ty_>
constexpr bool is_optional_v = _is_optional<std::remove_const_t<std::remove_reference_t<Ty_>>>::value;

template <size_t N_, typename Fn_, typename... Args_>
constexpr void visit_with_key(
        std::array<std::string, N_> const& keys,
        Fn_&& search,
        Args_&... args)
{
    size_t at = 0;
    (search(keys[at++], std::forward<Args_>(args)), ...);
}

constexpr auto from_json_visitor = [](auto&& r) {
    return [&](auto&& key, auto&& var) {
        constexpr bool is_optional = is_optional_v<decltype(var)>;
        if constexpr (is_optional)
        {
            auto it = r.find(key);
            if (it != r.end())
                ((not var.has_value() && var.emplace(), 0), it->get_to(*var));
        }
        else
        {
            r.at(key).get_to(var);
        }
    };
};

constexpr auto to_json_visitor = [](auto&& r) {
    return [&](auto&& key, auto&& var) {
        constexpr bool is_optional = is_optional_v<decltype(var)>;

        if constexpr (is_optional)
            var.has_value() && (r[key] = *var, 0);
        else
            r[key] = var;
    };
};

}  // namespace CPPHEADERS_NS_::archiving::detail
