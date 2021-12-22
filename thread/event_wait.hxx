#pragma once
#include <condition_variable>
#include <mutex>

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_::thread {
using namespace std::literals;

const auto event_wait_default_predicate = [] { return true; };
const auto event_wait_default_critical  = [] { return true; };

/**
 * Simple wrapper of event wait handle
 */
class event_wait
{
   private:
    using mutex_type = std::mutex;
    using cvar_type  = std::condition_variable;

    using ulock_type = std::unique_lock<mutex_type>;
    using pred_type  = decltype(event_wait_default_predicate);
    using opr_type   = decltype(event_wait_default_critical);

   public:
    template <typename Opr_ = opr_type>
    void notify_one(Opr_&& critical_proc)
    {
        ulock_type lc{_mtx};
        critical_proc();
        _cvar.notify_one();
    }

    template <typename Opr_ = opr_type>
    void notify_all(Opr_&& critical_proc)
    {
        ulock_type lc{_mtx};
        critical_proc();
        _cvar.notify_all();
    }

    template <typename Opr_ = opr_type>
    void notify_all_at_thread_exit(Opr_&& critical_proc)
    {
        ulock_type lc{_mtx};
        critical_proc();
        std::notify_all_at_thread_exit(_cvar, std::move(lc));
    }

    void notify_one()
    {
        ulock_type lc{_mtx};
        _cvar.notify_one();
    }

    void notify_all()
    {
        ulock_type lc{_mtx};
        _cvar.notify_all();
    }

    void notify_all_at_thread_exit()
    {
        ulock_type lc{_mtx};
        std::notify_all_at_thread_exit(_cvar, std::move(lc));
    }

    template <typename Pred_>
    void wait(Pred_&& predicate) const
    {
        ulock_type lc{_mtx};
        _cvar.wait(lc, std::forward<Pred_>(predicate));
    }

    void wait() const
    {
        ulock_type lc{_mtx};
        _cvar.wait(lc);
    }

    template <typename Duration_, typename Pred_>
    auto wait_for(Duration_&& duration, Pred_&& predicate) const
    {
        ulock_type lc{_mtx};
        return _cvar.wait_for(lc, std::forward<Duration_>(duration), std::forward<Pred_>(predicate));
    }

    template <typename Duration_>
    auto wait_for(Duration_&& duration) const
    {
        ulock_type lc{_mtx};
        return _cvar.wait_for(lc, std::forward<Duration_>(duration));
    }

    template <typename TimePoint_, typename Predicate_>
    auto wait_until(TimePoint_&& time_point, Predicate_&& predicate) const
    {
        ulock_type lc{_mtx};
        return _cvar.wait_until(
                lc,
                std::forward<TimePoint_>(time_point),
                std::forward<Predicate_>(predicate));
    }

    template <typename TimePoint_>
    auto wait_until(TimePoint_&& time_point) const
    {
        ulock_type lc{_mtx};
        return _cvar.wait_until(lc, std::forward<TimePoint_>(time_point));
    }

    auto lock() const { return ulock_type{_mtx}; }

   private:
    mutable std::condition_variable _cvar;
    mutable std::mutex _mtx;
};

}  // namespace CPPHEADERS_NS_::thread
