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