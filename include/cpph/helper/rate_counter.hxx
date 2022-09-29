#pragma once
#include <cpph/std/optional>

#include "cpph/utility/templates.hxx"
#include "cpph/utility/timer.hxx"

namespace cpph {
template <typename T, typename D = T,
          class = enable_if_t<
                  std::is_arithmetic_v<T>  //
                  &&                       //
                  std::is_arithmetic_v<T>  //
                  >>
class rate_counter
{
   public:
    using value_type = T;
    using delta_type = D;
    using const_reference = T const&;

   private:
    value_type current_ = {};
    value_type prev_ = {};
    delta_type latest_delta_ = {};
    delta_type latest_bw_ = {};
    poll_timer tmr_;

   public:
    rate_counter() noexcept = default;

    template <class R, class P>
    explicit rate_counter(duration<R, P> interval) noexcept : tmr_(interval)
    {
    }

    auto current() const noexcept -> const_reference { return current_; }
    auto prev() const noexcept -> const_reference { return prev_; }

   public:
    template <class R, class P>
    auto reset_interval(std::chrono::duration<R, P> const& new_interval)
            -> std::chrono::duration<R, P>
    {
        tmr_.reset(new_interval);
    }

    void add(const_reference value) noexcept
    {
        current_ += value;
    }

    void update(const_reference value) noexcept
    {
        current_ = value;
    }

    void reset(value_type value) noexcept
    {
        current_ = prev_ = move(value);
    }

    auto tick() noexcept -> optional<delta_type>
    {
        if (tmr_.check_sparse()) {
            latest_delta_ = current_ - prev_;
            prev_ = current_;

            return (latest_bw_ = delta_type(latest_delta_ / tmr_.delta().count()));
        }

        return {};
    }

    auto delta_time() const noexcept -> decltype(tmr_.delta())
    {
        return tmr_.delta();
    }

    auto const& delta() const noexcept { return latest_delta_; }
    auto const& rate() const noexcept { return latest_bw_; }
};

}  // namespace cpph
