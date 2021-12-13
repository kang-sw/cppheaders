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

    bool operator()() noexcept
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

    duration dt() const noexcept
    {
        return _latest_dt;
    }

    std::chrono::duration<double> delta() const noexcept
    {
        return dt();
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

   private:
    timepoint _tp       = {};
    duration _interval  = {};
    duration _latest_dt = {};
};

using poll_timer = basic_poll_timer<std::chrono::steady_clock>;
}  // namespace CPPHEADERS_NS_