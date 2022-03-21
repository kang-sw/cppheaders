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
#include <list>

#include "../detail/primitives.hxx"

//
#include "../detail/_init_macros.hxx"

namespace CPPHEADERS_NS_::refl {
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::nanoseconds;
using std::chrono::seconds;

class time_t
{
    int64_t  _sec;
    uint32_t _ns;

   public:
    constexpr time_t() noexcept : _sec{}, _ns{} {}

    template <typename Ref_, typename Ratio_>
    constexpr time_t(duration<Ref_, Ratio_> duration)
            : _sec(_retrieve_sec(duration)),
              _ns(_retrieve_ns(duration))
    {
    }

    constexpr time_t(int64_t sec, uint32_t ns) noexcept
            : _sec(sec), _ns(ns % unsigned(1e9))
    {
    }

   private:
    template <typename Ref_, typename Ratio_>
    constexpr static auto _retrieve_sec(duration<Ref_, Ratio_> dur) noexcept
    {
        using namespace std::literals;
        using dur_t = decltype(dur);

        auto sec = duration_cast<dur_t>(1s);
        auto div = int64_t(dur / sec);

        return div;
    }

    template <typename Ref_, typename Ratio_>
    constexpr static auto _retrieve_ns(duration<Ref_, Ratio_> dur) noexcept
    {
        using namespace std::literals;
        using dur_t = decltype(dur);

        auto sec = duration_cast<dur_t>(1s);
        auto div = int64_t(dur / sec);
        auto mod = dur - sec * div;
        return uint32_t(duration_cast<nanoseconds>(mod < dur_t(0) ? -mod : mod).count());
    }
};

INTERNAL_CPPH_define_(ValTy_, (is_template_instance_of<ValTy_, std::chrono::duration>::value))
{
    return detail::get_list_like_descriptor<ValTy_>();
}
}  // namespace CPPHEADERS_NS_::refl

#include "../detail/_deinit_macros.hxx"
