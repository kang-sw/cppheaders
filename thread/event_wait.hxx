#pragma once
#include <condition_variable>
#include <mutex>

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_::thread {
using namespace std::literals;

constexpr auto event_wait_default_predicate = [] { return true; };
constexpr auto event_wait_default_critical  = [] { return true; };

/**
 * Simple wrapper of event wait handle
 */
class event_wait
{
   private:
    using mutex_type = std::mutex;
    using cvar_type  = std::condition_variable;

    using ulock_type = std::unique_lock<mutex_type>;
    using pred_type  = bool (*)();
    using opr_type   = void (*)();

   public:
    template <typename Opr_ = opr_type>
    void notify_one(Opr_&& critical_proc = event_wait_default_critical)
    {
        ulock_type lc{_mtx};
        critical_proc();
        _cvar.notify_one();
    }

    template <typename Opr_ = opr_type>
    void notify_all(Opr_&& critical_proc = event_wait_default_critical)
    {
        ulock_type lc{_mtx};
        critical_proc();
        _cvar.notify_all();
    }

    template <typename Opr_ = opr_type>
    void notify_on_thread_exit(Opr_&& critical_proc = event_wait_default_critical)
    {
        ulock_type lc{_mtx};
        critical_proc();
        notify_all_at_thread_exit(_cvar, std::move(lc));
    }

    template <typename Pred_ = pred_type>
    void wait(Pred_&& predicate = event_wait_default_predicate)
    {
        ulock_type lc{_mtx};
        _cvar.wait(lc, std::forward<Pred_>(predicate));
    }

    template <typename Duration_, typename Pred_ = pred_type>
    bool wait_for(Duration_ duration, Pred_&& predicate = event_wait_default_predicate)
    {
        ulock_type lc{_mtx};
        return _cvar.wait_for(duration, std::forward<Pred_>(predicate));
    }

    template <typename TimePoint_, typename Predicate_ = pred_type>
    bool wait_until(TimePoint_ time_point, Predicate_&& predicate = event_wait_default_predicate)
    {
        ulock_type lc{_mtx};
        return _cvar.wait_until(time_point, std::forward<Predicate_>(predicate));
    }

    auto get_lock() const { return ulock_type{_mtx}; }

   private:
    std::condition_variable _cvar;
    mutable std::mutex _mtx;
};

}  // namespace CPPHEADERS_NS_::thread
