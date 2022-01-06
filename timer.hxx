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
#include <chrono>

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_ {
template <typename Clock_ = std::chrono::steady_clock>
class basic_poll_timer
{
   public:
    using clock_type = Clock_;
    using timepoint  = typename clock_type::time_point;
    using duration   = typename clock_type::duration;

   public:
    template <typename Duration_>
    basic_poll_timer(Duration_&& dt) noexcept
    {
        this->reset(std::forward<Duration_>(dt));
    }

    basic_poll_timer() noexcept
    {
        this->reset();
    }

    [[deprecated]] bool operator()() noexcept
    {
        return check();
    }

    // Check for intensive update call
    bool check() noexcept
    {
        auto now = clock_type::now();
        if (now > _tp)
        {
            _latest_dt = now - (_tp - _interval);
            _tp += _interval;

            if (_tp < now)
                _tp = now;  // prevent burst

            return true;
        }
        else
        {
            return false;
        }
    }

    // Check for sparse event call
    bool check_sparse() noexcept
    {
        auto now = clock_type::now();
        if (now > _tp)
        {
            _latest_dt = now - (_tp - _interval);
            _tp        = now + _interval;

            return true;
        }
        else
        {
            return false;
        }
    }

    duration dt() const noexcept
    {
        return _latest_dt;
    }

    std::chrono::duration<double> delta() const noexcept
    {
        return dt();
    }

    duration remaining() const noexcept
    {
        return std::max(_tp - clock_type::now(), duration{});
    }

    template <typename Duration_>
    void reset(Duration_&& interval) noexcept
    {
        _interval = std::chrono::duration_cast<duration>(std::forward<Duration_>(interval));
        this->reset();
    }

    void reset() noexcept
    {
        _tp = clock_type::now() + _interval;
    }

    void invalidate() noexcept
    {
        _tp = clock_type::now();
    }

    auto interval() const noexcept
    {
        return _interval;
    }

    auto interval_sec() const noexcept
    {
        return std::chrono::duration<double>(_interval);
    }

   private:
    timepoint _tp       = {};
    duration _interval  = {};
    duration _latest_dt = {};
};

class stopwatch
{
   public:
    using clock_type = std::chrono::steady_clock;

   public:
    stopwatch() noexcept { reset(); }
    void reset() noexcept { _tp = clock_type::now(); }
    auto tick() const noexcept { return clock_type::now() - _tp; }
    auto elapsed() const noexcept { return std::chrono::duration<double>(tick()); }

   private:
    clock_type::time_point _tp;
};

using poll_timer = basic_poll_timer<std::chrono::steady_clock>;

namespace utilities {
using namespace std::literals;
}
}  // namespace CPPHEADERS_NS_
