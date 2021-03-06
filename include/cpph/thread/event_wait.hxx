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
#include <condition_variable>
#include <mutex>

#include "cpph/memory/pool.hxx"

//

namespace cpph {
using std::condition_variable;
using std::mutex;
using std::timed_mutex;
using std::unique_lock;
}  // namespace cpph

namespace cpph::thread {
using namespace std::literals;

const auto event_wait_default_predicate = [] { return true; };
const auto event_wait_default_critical = [] { return true; };

/**
 * Simple wrapper of event wait handle
 */
class event_wait
{
   private:
    mutable std::condition_variable _cvar;
    mutable std::mutex _mtx;

   private:
    using mutex_type = std::mutex;
    using cvar_type = std::condition_variable;

    using ulock_type = std::unique_lock<mutex_type>;
    using pred_type = decltype(event_wait_default_predicate);
    using opr_type = decltype(event_wait_default_critical);

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
        _cvar.notify_one();
    }

    void notify_all()
    {
        _cvar.notify_all();
    }

    void notify_all_at_thread_exit()
    {
        ulock_type lc{_mtx};
        std::notify_all_at_thread_exit(_cvar, std::move(lc));
    }

    template <typename Pp_, typename Pred_>
    auto wait_pp(Pp_&& preproc, Pred_&& predicate) const
    {
        ulock_type lc{_mtx};
        preproc();
        _cvar.wait(lc, std::forward<Pred_>(predicate));

        return lc;
    }

    template <typename Pp_>
    auto wait_pp(Pp_&& preproc) const
    {
        ulock_type lc{_mtx};
        preproc();
        _cvar.wait(lc);

        return lc;
    }

    template <typename Pp_, typename Pred_, typename Dur_>
    auto wait_pp_for(Dur_&& duration, Pp_&& preproc, Pred_&& predicate) const
    {
        ulock_type lc{_mtx};
        preproc();

        if (not _cvar.wait_for(lc, std::forward<Dur_>(duration), std::forward<Pred_>(predicate)))
            lc.unlock();

        return lc;
    }

    template <typename Pp_, typename Dur_>
    auto wait_pp_for(Dur_&& duration, Pp_&& preproc) const
    {
        ulock_type lc{_mtx};
        preproc();

        if (not _cvar.wait_for(lc, std::forward<Dur_>(duration)))
            lc.unlock();

        return lc;
    }

    template <typename Pp_, typename Pred_, typename Dur_>
    auto wait_pp_until(Dur_&& timeout, Pp_&& preproc, Pred_&& predicate) const
    {
        ulock_type lc{_mtx};
        preproc();

        if (not _cvar.wait_until(lc, std::forward<Dur_>(timeout), std::forward<Pred_>(predicate)))
            lc.unlock();

        return lc;
    }

    template <typename Pp_, typename Dur_>
    auto wait_pp_until(Dur_&& timeout, Pp_&& preproc) const
    {
        ulock_type lc{_mtx};
        preproc();

        if (not _cvar.wait_until(lc, std::forward<Dur_>(timeout)))
            lc.unlock();

        return lc;
    }

    template <typename Pred_>
    auto wait(Pred_&& predicate) const
    {
        ulock_type lc{_mtx};
        _cvar.wait(lc, std::forward<Pred_>(predicate));

        return lc;
    }

    auto wait() const
    {
        ulock_type lc{_mtx};
        _cvar.wait(lc);

        return lc;
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

    template <typename Duration_, typename Pred_>
    auto wait_for_2(Duration_&& duration, Pred_&& predicate) const
    {
        ulock_type lc{_mtx};
        if (_cvar.wait_for(lc, std::forward<Duration_>(duration), std::forward<Pred_>(predicate)))
            return lc;
        else
            return ulock_type{};
    }

    template <typename Duration_>
    auto wait_for_2(Duration_&& duration) const
    {
        ulock_type lc{_mtx};
        if (_cvar.wait_for(lc, std::forward<Duration_>(duration)))
            return lc;
        else
            return ulock_type{};
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

    template <typename TimePoint_, typename Predicate_>
    auto wait_until_2(TimePoint_&& time_point, Predicate_&& predicate) const
    {
        ulock_type lc{_mtx};
        auto b = _cvar.wait_until(
                lc,
                std::forward<TimePoint_>(time_point),
                std::forward<Predicate_>(predicate));

        return b ? std::move(lc) : ulock_type{};
    }

    template <typename TimePoint_>
    auto wait_until_2(TimePoint_&& time_point) const
    {
        ulock_type lc{_mtx};
        auto b = _cvar.wait_until(lc, std::forward<TimePoint_>(time_point));
        return b ? lc : ulock_type{};
    }

    template <typename CriticalOp_>
    void critical_section(CriticalOp_&& op)
    {
        std::lock_guard _{_mtx};
        op();
    }

    auto lock() const { return ulock_type{_mtx}; }
    auto& mutex() const { return _mtx; }
};

/**
 * Used for instant wait of async events ...
 */
class trigger_wait
{
    // TODO: In c++ 20, change this to use atomic wait

   private:
    inline static pool<event_wait> _reuse;
    pool_ptr<event_wait> _body;
    bool _ready = false;

   public:
    explicit trigger_wait(nullptr_t) noexcept : _body(nullptr) {}
    explicit trigger_wait() : _body(_reuse.checkout()) {}

   public:
    void prepare()
    {
        _body = _reuse.checkout();
    }

    void trigger()
    {
        assert(_body);
        _body->notify_one([this] { _ready = true; });
    }

    void wait()
    {
        assert(_body);
        _body->wait([this] { return _ready; });
    }

    void reset()
    {
        _body->critical_section([this] { _ready = false; });
    }
};

class scoped_trigger
{
    trigger_wait* _wait;

   public:
    scoped_trigger(trigger_wait& wait) noexcept : _wait(&wait) {}
    ~scoped_trigger() { _wait->trigger(); }
};

}  // namespace cpph::thread
