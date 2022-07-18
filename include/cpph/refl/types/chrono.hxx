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
#include <chrono>
#include <cpph/std/list>

#include "../detail/primitives.hxx"

//
#include "../detail/object_primitive_macros.hxx"

namespace cpph {
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::nanoseconds;
using std::chrono::seconds;
}  // namespace cpph

namespace cpph::refl {

#pragma pack(push, 4)
class time_info
{
    int64_t _sec;
    uint32_t _ns;

   public:
    constexpr time_info() noexcept : _sec{}, _ns{} {}

    template <typename Ref_, typename Ratio_>
    constexpr time_info(duration<Ref_, Ratio_> duration)
            : _sec(_retrieve_sec(duration)),
              _ns(_retrieve_ns(duration))
    {
    }

    constexpr time_info(int64_t sec, uint32_t ns) noexcept
            : _sec(sec), _ns(ns % unsigned(1e9))
    {
    }

    template <typename Ref_, typename Ratio_>
    void retrieve(duration<Ref_, Ratio_>& out)
    {
        using out_t = decay_t<decltype(out)>;
        auto delta_ns = duration_cast<out_t>(int64_t(_ns) * 1ns);

        out = duration_cast<out_t>(_sec * 1s);
        out += delta_ns * (_sec >= 0 ? 1 : -1);
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
static_assert(sizeof(time_info) == 12);
#pragma pack(pop)
}  // namespace cpph::refl

CPPH_REFL_DEFINE_PRIM_T_begin(ValueType, is_template_instance_of<ValueType, std::chrono::duration>::value)
{
    CPPH_REFL_primitive_type(binary);

    CPPH_REFL_primitive_archive(strm, data)
    {
        time_info t{data};
        strm->binary_push(sizeof t);
        strm->binary_write_some({&t, 1});
        strm->binary_pop();
    }

    CPPH_REFL_primitive_restore(strm, pvdata)
    {
        time_info t;
        {
            auto n = strm->begin_binary();

            if (n != sizeof(time_info)) {
                strm->end_binary();
                throw archive::error::reader_check_failed{strm};
            }

            strm->binary_read_some({&t, 1});
            strm->end_binary();
        }

        t.retrieve(*pvdata);
    }
}
CPPH_REFL_DEFINE_PRIM_T_end();
