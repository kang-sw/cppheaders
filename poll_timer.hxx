#pragma once
#include <chrono>

namespace KANGSW_TEMPLATE_NAMESPACE {
template <typename Clock_ = std::chrono::steady_clock>
class poll_timer {
 public:
  using clock_type = Clock_;
  using timepoint  = typename clock_type::time_point;
  using duration   = typename clock_type::duration;

 public:
  template <typename Duration_>
  poll_timer(Duration_&& dt) noexcept {
    this->reset(std::forward<Duration_>(dt));
  }

  poll_timer() noexcept {
    this->reset();
  }

  bool operator()() noexcept {
    auto now = clock_type::now();
    if (now > _tp) {
      _tp += _interval;

      if (_tp < now)
        _tp = now;  // prevent burst

      return true;
    } else {
      return false;
    }
  }

  template <typename Duration_>
  void reset(Duration_&& interval) noexcept {
    _interval = std::chrono::duration_cast<duration>(std::forward<Duration_>(interval));
    this->reset();
  }

  void reset() noexcept {
    _tp = clock_type::now() + _interval;
  }

 private:
  timepoint _tp      = {};
  duration _interval = {};
};

}  // namespace KANGSW_TEMPLATE_NAMESPACE