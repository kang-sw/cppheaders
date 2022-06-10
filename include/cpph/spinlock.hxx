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

#include "threading.hxx"

namespace cpph {
//! @see https://rigtorp.se/spinlock/
//! Applied slight modification to use atomic_flag
struct spinlock {
    std::atomic_bool lock_{false};

    //
    void lock() noexcept
    {
        for (;;) {
            // Optimistically assume the lock is free on the first try
            if (!lock_.exchange(true, std::memory_order_acquire)) {
                return;
            }
            // Wait for lock to be released without generating cache misses
            while (lock_.load(std::memory_order_relaxed)) {
                // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
                // hyper-threads
                _detail::thread_yield();
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

//! Shared spinlock
struct shared_spinlock {
    atomic<int> _nread{0};
    atomic<bool> _write{false};

    void lock() noexcept
    {
        // SEE: spinlock::lock()
        for (;;) {
            if (not _write.exchange(false, std::memory_order_acquire))
                break;

            while (_write.load(std::memory_order_relaxed))
                _detail::thread_yield();
        }

        // Wait until all read access is disposed
        while (relaxed(_nread) != 0) {
            _detail::thread_yield();
        }
    }

    void unlock() noexcept
    {
        _write.store(false, std::memory_order_release);
    }

    void lock_shared() noexcept
    {
        for (;;) {
            // Wait write lock released
            while (relaxed(_write)) {
                _detail::thread_yield();
            }

            // Lock shared access
            _nread.fetch_add(1, memory_order_acquire);

            if (relaxed(_write)) {
                // Write lock acquired during transaction ... Go another round.
                _nread.fetch_sub(1, memory_order_release);
            } else {
                break;
            }
        }
    }

    void unlock_shared() noexcept
    {
        _nread.fetch_sub(1, memory_order_release);
    }
};

}  // namespace cpph
