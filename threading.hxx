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

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_ {
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

struct null_mutex
{
    bool try_lock() noexcept { return true; }
    void lock() noexcept {}
    void unlock() noexcept {}
};

}  // namespace CPPHEADERS_NS_