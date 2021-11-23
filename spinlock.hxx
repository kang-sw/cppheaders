/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <ki6080@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.      Seungwoo Kang.
 * ----------------------------------------------------------------------------
 */
#pragma once
#include <atomic>
#include <thread>

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_
{
// lock guard utility
template <typename Mutex_>
struct lock_guard
{
    enum
    {
        nothrow_lockable = std::is_nothrow_invocable_v<decltype(&Mutex_::lock), Mutex_*>
    };
    enum
    {
        nothrow_unlockable = std::is_nothrow_invocable_v<decltype(&Mutex_::unlock), Mutex_*>
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

//! @see https://rigtorp.se/spinlock/
//! Applied slight modification to use atomic_flag
struct spinlock
{
    std::atomic_bool lock_{false};

    void lock() noexcept
    {
        for (;;)
        {
            // Optimistically assume the lock is free on the first try
            if (!lock_.exchange(true, std::memory_order_acquire))
            {
                return;
            }
            // Wait for lock to be released without generating cache misses
            while (lock_.load(std::memory_order_relaxed))
            {
                // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
                // hyper-threads
                std::this_thread::yield();
            }
        }
    }

    bool try_lock() noexcept
    {
        // First do a relaxed load to check if lock is free in order to prevent
        // unnecessary cache misses if someone does while(!try_lock())
        return !lock_.load(std::memory_order_relaxed) && !lock_.exchange(true, std::memory_order_acquire);
    }

    void unlock() noexcept
    {
        lock_.store(false, std::memory_order_release);
    }
};
}  // namespace CPPHEADERS_NS_
