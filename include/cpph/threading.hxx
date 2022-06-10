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
#include <atomic>

#if _MSVC_LANG
void __cdecl _Thrd_yield();
#elif linux
extern int sched_yield(void) __THROW;
#endif

namespace cpph::_detail {

inline void thread_yield()
{
#if _MSVC_LANG
    _Thrd_yield();
#elif linux
    sched_yield();
#endif
}

}  // namespace cpph::_detail

namespace cpph {
using std::atomic;
using std::atomic_bool;
using std::atomic_flag;
using std::atomic_int;
using std::atomic_size_t;

using std::memory_order_acq_rel;
using std::memory_order_acquire;
using std::memory_order_consume;
using std::memory_order_relaxed;
using std::memory_order_release;
using std::memory_order_seq_cst;

// lock guard utility
template <typename Mutex_>
struct lock_guard {
    enum {
        nothrow_lockable = noexcept(std::declval<Mutex_>().lock()),
        nothrow_unlockable = noexcept(std::declval<Mutex_>().unlock())
    };

    lock_guard(Mutex_& mtx) noexcept(nothrow_lockable) : _ref(mtx)
    {
        _ref.lock();
    }

    void lock() noexcept(nothrow_lockable)
    {
        _ref.lock();
        _locked = true;
    }

    void unlock() noexcept(nothrow_unlockable)
    {
        _ref.unlock();
        _locked = false;
    }

    ~lock_guard() noexcept(nothrow_unlockable)
    {
        if (_locked)
            _ref.unlock();
    }

    bool _locked = true;
    Mutex_& _ref;
};

struct null_mutex {
    bool try_lock() noexcept { return true; }
    void lock() noexcept {}
    void unlock() noexcept {}
};

static_assert(noexcept(std::declval<null_mutex>().lock()));
static_assert(noexcept(std::declval<null_mutex>().try_lock()));
static_assert(noexcept(std::declval<null_mutex>().unlock()));

//! Atomic utility
template <typename ValTy>
ValTy acquire(std::atomic<ValTy> const& value) noexcept
{
    return value.load(std::memory_order_acquire);
}

template <typename ValTy, typename Rhs>
void release(std::atomic<ValTy>& value, Rhs&& other) noexcept
{
    value.store(std::forward<Rhs>(other), std::memory_order_release);
}

template <typename ValTy>
ValTy relaxed(std::atomic<ValTy> const& value) noexcept
{
    return value.load(std::memory_order_relaxed);
}

template <typename ValTy, typename Rhs>
void relaxed(std::atomic<ValTy>& value, Rhs&& other) noexcept
{
    return value.store(std::forward<Rhs>(other), std::memory_order_relaxed);
}

template <typename ValTy, typename Rhs>
ValTy fetch_add_relaxed(std::atomic<ValTy> const& value, Rhs&& other) noexcept
{
    return value.fetch_add(std::memory_order_relaxed, std::forward<Rhs>(other));
}

template <typename ValTy, typename Rhs>
ValTy fetch_add_acq_rel(std::atomic<ValTy>& value, Rhs&& other) noexcept
{
    return value.fetch_add(std::forward<Rhs>(other), std::memory_order_acq_rel);
}
}  // namespace cpph