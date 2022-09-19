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
#if __has_include(<fmt/format.h>)
#    include <fmt/format.h>
#elif __has_include(<spdlog/fmt/fmt.h>)
#    include <spdlog/fmt/fmt.h>
#endif

//

namespace cpph {
namespace util {
class format_context
{
   public:
    template <typename... Args_>
    struct _proxy {
        using tuple_type = std::tuple<Args_...>;

        format_context* _base;
        tuple_type _tup;

       private:
        template <typename Ty_, size_t... Idx_>
        auto _concat_tuple(Ty_&& arg, std::index_sequence<Idx_...>) noexcept
        {
            return std::forward_as_tuple(std::get<Idx_>(std::move(_tup))..., std::forward<Ty_>(arg));
        }

       public:
        template <typename Ty_>
        auto operator%(Ty_&& arg) noexcept
        {
            using indices_t
                    = std::make_index_sequence<std::tuple_size_v<tuple_type>>;

            return _proxy<Args_..., decltype(arg)>{
                    _base, _concat_tuple(std::forward<Ty_>(arg), indices_t{})};
        }

        auto& operator>>(std::string& arg) noexcept
        {
            std::apply(
                    [&](auto&&... args) {
                        fmt::format_to(std::back_inserter(arg), _base->_fmt,
                                       std::forward<decltype(args)>(args)...);
                    },
                    std::move(_tup));
            return arg;
        }

        auto&& operator>>(std::string&& arg) noexcept
        {
            return std::move((*this) >> (reinterpret_cast<std::string&>(arg)));
        }

        auto& operator>(std::string& arg) noexcept
        {
            arg.clear();
            std::apply(
                    [&](auto&&... args) {
                        fmt::format_to(std::back_inserter(arg), _base->_fmt,
                                       std::forward<decltype(args)>(args)...);
                    },
                    std::move(_tup));
            return arg;
        }

        auto&& operator>(std::string&& arg) noexcept
        {
            return std::move((*this) > (reinterpret_cast<std::string&>(arg)));
        }

        auto operator/(size_t init_cap) noexcept
        {
            std::string str;
            str.reserve(init_cap);
            return (*this) > str;
        }

        template <typename... Args2_>
        std::string operator()(Args2_&&... args)
        {
            return fmt::format(fmt::runtime(_base->_fmt), std::forward<Args2_>(args)...);
        }

        operator std::string()
        {
            return std::apply(*this, std::move(_tup));
        }

        std::string string()
        {
            return std::apply(*this, std::move(_tup));
        }
    };

   public:
    // format with arbitrary types
    template <typename Ty_>
    auto operator%(Ty_&& arg) noexcept
    {
        return _proxy<decltype(arg)>{this, std::forward_as_tuple(std::forward<Ty_>(arg))};
    }

    template <typename... Args_>
    auto operator()(Args_&&... args) noexcept
    {
        return _proxy<decltype(args)...>{this, std::forward_as_tuple(std::forward<Args_>(args)...)};
    }

    _proxy<> s() noexcept { return _proxy<>{this}; }

   public:
    char const* const _fmt = {};
};

template <typename Str_, typename... Args_>
auto&& operator<(Str_&& str, format_context::_proxy<Args_...>&& prx)
{
    return prx.operator>(std::forward<Str_>(str));
}

template <typename Str_, typename... Args_>
auto&& operator<<(Str_&& str, format_context::_proxy<Args_...>&& prx)
{
    return prx.operator>>(std::forward<Str_>(str));
}

template <typename... Args_>
std::string to_string(format_context::_proxy<Args_...>&& prx)
{
    return prx.string();
}

}  // namespace util

class format_buffer : public std::string
{
   public:
    using std::string::basic_string;

    template <typename Fmt_, typename... Args_>
    std::string& format(Fmt_&& fmt, Args_&&... args)
    {
        this->clear();
        return format_append(std::forward<Fmt_>(fmt), std::forward<Args_>(args)...);
    }

    template <typename Fmt_, typename... Args_>
    std::string& format_append(Fmt_&& fmt, Args_&&... args)
    {
        fmt::format_to(
                std::back_inserter(*this),
                fmt::runtime(std::forward<Fmt_>(fmt)),
                std::forward<Args_>(args)...);

        return *this;
    }
};

inline namespace literals {
using namespace std::literals;
inline util::format_context operator""_fmt(char const* ch, size_t)
{
    return {ch};
}
}  // namespace literals
}  // namespace cpph

namespace cpph::utilities {
using namespace cpph::literals;
}  // namespace cpph::utilities
