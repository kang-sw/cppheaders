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
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <cpph/std/vector>

#include "cpph/thread/spinlock.hxx"
#include "cpph/thread/threading.hxx"
#include "functional.hxx"
#include "hasher.hxx"

//

namespace cpph {
using event_key = basic_key<class LABEL_delegate_key>;

enum class event_control {
    ok = 0,
    expire = 1,
    consume = 2,
};

inline event_control operator|(event_control a, event_control b)
{
    return (event_control)(int(a) | int(b));
}

enum {
    DELEGATE_BITS = 61
};

enum class event_priority : uint64_t {
    last = 0,
    very_low = 1ull << DELEGATE_BITS,
    low = 2ull << DELEGATE_BITS,
    middle = 3ull << DELEGATE_BITS,
    high = 4ull << DELEGATE_BITS,
    very_high = 5ull << DELEGATE_BITS,
    first = ~uint64_t{},
};

template <typename Mutex_, typename... Args_>
class basic_event
{
   public:
    using event_fn = ufunction<event_control(Args_...)>;

    struct _entity_type {
        event_key id;
        std::shared_ptr<event_fn> ufunction;
        uint64_t priority = 0;

        bool operator<(_entity_type const& rhs) const noexcept
        {
            // send remove candidates to back
            if (not id && rhs.id)
                return false;
            if (id && not rhs.id)
                return true;

            return priority > rhs.priority;
        }
    };

    static_assert(std::is_nothrow_move_assignable_v<_entity_type>);
    static_assert(std::is_nothrow_move_constructible_v<_entity_type>);

   public:
    using container = std::vector<_entity_type>;
    using mutex_type = Mutex_;

   public:
    class handle
    {
        friend class basic_event;
        basic_event* owner_ = {};
        event_key key_ = {};

        handle(basic_event* o, event_key key) noexcept
                : owner_{o}, key_{key} {}

       public:
        auto& operator=(handle&& o) noexcept
        {
            owner_ = o.owner_;
            key_ = o.key_;
            o.owner_ = nullptr;
            o.key_ = {};
            return *this;
        }

        handle(handle&& other) noexcept { (*this) = std::move(other); }
        void expire() noexcept { owner_->remove(std::move(*this)); }
        bool valid() noexcept { return owner_ != nullptr; }
    };

   public:
    basic_event() noexcept = default;
    basic_event(basic_event&& rhs) noexcept { *this = std::move(rhs); }
    basic_event(basic_event const& rhs) noexcept { *this = rhs; }

    basic_event& operator=(basic_event&& rhs) noexcept
    {
        lock_guard a{_mtx}, b{rhs._mtx};
        _events = std::move(rhs._events);
        _mtx.unlock(), rhs._mtx.unlock();
    }

    basic_event& operator=(basic_event const& rhs) noexcept
    {
        lock_guard a{_mtx}, b{rhs._mtx};
        _events = rhs._events;
        _mtx.unlock(), rhs._mtx.unlock();
    }

    template <typename... Params>
    void invoke(Params&&... args)
    {
        lock_guard lock{_mtx};

        if (_dirty) {
            _dirty = false;
            std::sort(_events.begin(), _events.end());
        }

        for (auto idx = 0; idx < _events.size();) {
            if (not _events[idx].id) {
                _events.erase(_events.begin() + idx);
            } else {
                auto func_ptr = _events[idx].ufunction;
                lock.unlock();
                auto invoke_result = (int)(*func_ptr)(forward<Params>(args)...);
                lock.lock();

                if (invoke_result & (int)event_control::expire)
                    _events.erase(_events.begin() + idx);
                else
                    ++idx;

                if (invoke_result & (int)event_control::consume)
                    break;
            }
        }
    }

    template <typename Callable_>
    handle add(Callable_&& fn,
               event_priority priority = event_priority::middle,
               int64_t value = 0)
    {
        lock_guard _{_mtx};
        auto* evt = &_events.emplace_back();
        evt->id = {++_hash_gen};
        evt->priority = value + (uint64_t)priority;

        _dirty |= evt->priority != 0;

        using std::forward;
        using std::make_shared;

        if constexpr (std::is_invocable_r_v<event_control, Callable_, Args_...>) {
            evt->ufunction = make_shared<event_fn>(forward<Callable_>(fn));
        } else if constexpr (std::is_invocable_r_v<bool, Callable_, Args_...>) {
            evt->ufunction = make_shared<event_fn>(
                    [_fn = forward<Callable_>(fn)](auto&&... args) mutable {
                        return _fn(args...) ? event_control::ok
                                            : event_control::expire;
                    });
        } else if constexpr (std::is_invocable_v<Callable_, Args_...>) {
            evt->ufunction = make_shared<event_fn>(
                    [_fn = forward<Callable_>(fn)](auto&&... args) mutable {
                        return _fn(args...), event_control::ok;
                    });
        } else {
            evt->ufunction = make_shared<event_fn>(
                    [_fn = forward<Callable_>(fn)](auto&&...) mutable {
                        return _fn(), event_control::ok;
                    });
        }

        return handle{this, evt->id};
    }

    template <typename Ptr_, typename Callable_>
    handle add_weak(Ptr_&& ptr, Callable_&& callable, event_priority priority = event_priority::middle, int64_t value = 0)
    {
        std::weak_ptr wptr{std::forward<Ptr_>(ptr)};

        return add(
                [wptr = std::move(wptr),
                 callable = std::forward<Callable_>(callable)](
                        Args_... args) mutable
                -> event_control {
                    auto anchor = wptr.lock();  // Prevent anchor to be destroyed during function call
                    if (not anchor) { return event_control::expire; }

                    if constexpr (std::is_invocable_r_v<event_control, Callable_, Args_...>)
                        return callable(args...);
                    else if constexpr (std::is_invocable_v<Callable_, Args_...>)
                        return callable(args...), event_control::ok;
                    else if constexpr (std::is_invocable_r_v<event_control, Callable_>)
                        return callable();
                    else if constexpr (std::is_invocable_v<Callable_>)
                        return callable(), event_control::ok;
                    else
                        return ((args.INVALID), ...);
                },
                priority,
                value);
    }

    void priority(handle const& h, event_priority offset, int64_t value = 0) noexcept
    {
        lock_guard _{_mtx};
        auto entity = _find(h);
        if (not entity)
            return;

        value &= (1ull << DELEGATE_BITS) - 1;
        value += (uint64_t)offset;
        entity->priority = value;
        _dirty = true;
    }

    void remove(handle h) noexcept
    {
        lock_guard _{_mtx};
        auto entity = _find(h);
        if (not entity)
            return;

        entity->id = {};
        _dirty = true;
    }

    bool empty() const noexcept
    {
        lock_guard _{_mtx};
        return _events.empty();
    }

    bool size() const noexcept
    {
        lock_guard _{_mtx};
        return _events.size();
    }

    template <typename Callable_>
    auto& operator+=(Callable_&& rhs)
    {
        add(std::forward<Callable_>(rhs));
        return *this;
    }

    auto& operator-=(handle&& r) noexcept
    {
        remove(std::move(r));
        return *this;
    }

   private:
    _entity_type* _find(handle const& h)
    {
        if (not h.key_)
            throw std::logic_error{"invalid handle!"};

        auto it = std::find_if(
                _events.begin(), _events.end(),
                [&](_entity_type const& s) {
                    return h.key_ == s.id;
                });
        return it == _events.end() ? nullptr : &*it;
    }

   private:
    container _events;
    uint64_t _hash_gen = 0;

    mutable Mutex_ _mtx;
    volatile bool _dirty = false;
};

template <typename... Args_>
using event = basic_event<spinlock, Args_...>;

template <typename... Args_>
using event_st = basic_event<null_mutex, Args_...>;

}  // namespace cpph
