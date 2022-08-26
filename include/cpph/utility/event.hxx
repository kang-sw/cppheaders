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
#include <atomic>
#include <cpph/std/list>
#include <cpph/std/mutex>
#include <memory>
#include <stdexcept>

#include "cpph/thread/spinlock.hxx"
#include "cpph/thread/threading.hxx"
#include "functional.hxx"
#include "hasher.hxx"

//

namespace cpph {
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

template <typename Mutex, typename... Args>
class basic_event
{
   public:
    using event_fn = ufunction<event_control(Args...)>;
    using param_pack_t = tuple<conditional_t<std::is_reference_v<Args>, Args, Args const&>...>;

   private:
    struct if_entity {
       private:
        atomic<intptr_t> refcnt_ = 1;

       public:
        basic_event* owner = nullptr;
        uint64_t priority = 0;

        if_entity* p_prev = nullptr;
        if_entity* p_next = nullptr;

        virtual ~if_entity() = default;
        virtual event_control operator()(param_pack_t) = 0;

        void release() noexcept
        {
            if (refcnt_.fetch_sub(1, std::memory_order_release) == 1) {
                // If it's zero reference count, make it tiny value which makes further add_ref()
                //  request be ignored.
                constexpr auto SMALL_VALUE = numeric_limits<intptr_t>::min() / 2;
                intptr_t expected = 0;

                if (refcnt_.compare_exchange_strong(expected, SMALL_VALUE)) {
                    {  // Detach this node from siblings safely
                        [[maybe_unused]] scoped_lock _{owner->mtx_};

                        // Erase owner pointer
                        if_entity** pp_owner_node = nullptr;
                        if (this == owner->node_front_)
                            pp_owner_node = &owner->node_front_;
                        else if (this == owner->new_node_front_)
                            pp_owner_node = &owner->new_node_front_;

                        if (pp_owner_node != nullptr) {
                            *pp_owner_node = (*pp_owner_node)->p_next;
                            (*pp_owner_node)->p_prev = nullptr;
                        }

                        // Unlink nodes
                        p_prev->p_next = p_next;
                        p_next->p_prev = p_prev;

                        p_prev = p_next = nullptr;
                    }

                    // Erase self
                    delete this;
                }
            }
        }

        bool add_ref() noexcept
        {
            return refcnt_.fetch_add(1, std::memory_order_acquire) > 0;
        }
    };

   public:
    using mutex_type = Mutex;

   private:
    mutable Mutex mtx_;
    size_t size_ = 0;
    if_entity* node_front_ = nullptr;
    if_entity* new_node_front_ = nullptr;

   public:
    class handle
    {
        friend class basic_event;
        if_entity* node_ = nullptr;

        explicit handle(if_entity* p_node) noexcept
                : node_{p_node}
        {
            node_->add_ref();
        }

       public:
        ~handle() noexcept
        {
            node_->release();
        }

        auto& operator=(handle&& o) noexcept
        {
            swap(node_, o.node_);
            return *this;
        }

        handle(handle&& other) noexcept { (*this) = std::move(other); }
        bool valid() noexcept { return node_ != nullptr; }
        void expire() noexcept { exchange(node_, nullptr)->release(); }
    };

   public:
    basic_event() noexcept = default;
    basic_event(basic_event&& rhs) noexcept { *this = std::move(rhs); }

    basic_event& operator=(basic_event&& rhs) noexcept
    {
        std::scoped_lock _{mtx_, rhs.mtx_};
        swap(size_, rhs.size_);
        swap(node_front_, rhs.node_front_);
        swap(new_node_front_, rhs.new_node_front_);
    }

    template <typename... Params>
    void invoke(Params&&... args)
    {
        // TODO: Perform insertion
        {
            scoped_lock _{mtx_};

            while (new_node_front_ != nullptr) {
                auto new_node = new_node_front_;
                new_node->p_prev = nullptr;
                new_node->p_next = nullptr;
                new_node_front_ = new_node_front_->p_next;

                if (node_front_ != nullptr) {
                    auto p_insert = node_front_;
                    while (p_insert && p_insert->priority > new_node->priority) {
                        if (p_insert->p_next != nullptr) {
                            p_insert = p_insert->p_next;
                        } else {
                            p_insert->p_next = new_node;
                            new_node->p_prev = p_insert;

                            // done.
                            p_insert = nullptr;
                        }
                    }

                    if (p_insert) {
                        p_insert->p_next->p_prev = new_node;
                        new_node->p_next = p_insert->p_next;
                        p_insert->p_next = new_node;
                        new_node->p_prev = p_insert;
                    }
                } else {
                    node_front_ = new_node;
                }
            }
        }

        // TODO: Perform invocation
        for (if_entity* p_head;;) {

        }
    }

    template <typename Callable_>
    handle add(Callable_&& fn,
               event_priority priority = event_priority::middle,
               int64_t value = 0)
    {
        struct entity_t : if_entity {
            decay_t<Callable_> fn_;
            explicit entity_t(Callable_&& fn) : fn_(fn) {}

            event_control operator()(param_pack_t tup) override
            {
                return std::apply([this](auto&... args) -> event_control {
                    if constexpr (is_invocable_r_v<event_control, Callable_, Args...>) {
                        return fn_(args...);
                    } else if constexpr (is_invocable_r_v<bool, Callable_, Args...>) {
                        return fn_(args...) ? event_control::ok : event_control::expire;
                    } else if constexpr (is_invocable_v<Callable_, Args...>) {
                        return fn_(args...), event_control::ok;
                    } else if constexpr (is_invocable_v<Callable_>) {
                        return fn_(), event_control::ok;
                    } else {
                        Callable_::ERROR_NO_INVOCATION_AVAILABLE;
                    } }, tup);
            }
        };

        ptr<if_entity> p_new_node = make_unique<entity_t>(std::forward<Callable_>(fn));
        p_new_node->owner = this;
        p_new_node->priority = value + uint64_t(priority);

        {
            scoped_lock _{mtx_};
            auto p_node = p_new_node.release();
            p_node->p_next = new_node_front_, new_node_front_->p_prev = p_node, new_node_front_ = p_node;
            ++size_;

            return handle{new_node_front_};
        }
    }

    template <typename Ptr_, typename Callable_>
    handle add_weak(Ptr_&& ptr, Callable_&& callable, event_priority priority = event_priority::middle, int64_t value = 0)
    {
        std::weak_ptr wptr{std::forward<Ptr_>(ptr)};

        return add(
                [wptr = std::move(wptr),
                 callable = std::forward<Callable_>(callable)](
                        Args... args) mutable
                -> event_control {
                    auto anchor = wptr.lock();  // Prevent anchor to be destroyed during function call
                    if (not anchor) { return event_control::expire; }

                    if constexpr (std::is_invocable_r_v<event_control, Callable_, Args...>)
                        return callable(args...);
                    else if constexpr (std::is_invocable_v<Callable_, Args...>)
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
        if (h.node_->add_ref()) {
            {
                scoped_lock _{mtx_};
                h.node_->priority = value + (uint64_t)offset;

                // TODO: Replace this node
            }
            h.node_->release();
        }
    }

    void remove(handle h) noexcept
    {
        h.expire();
    }

    bool empty() const noexcept
    {
        lock_guard _{mtx_};
        return node_front_ == nullptr && new_node_front_ == nullptr;
    }

    bool size() const noexcept
    {
        lock_guard _{mtx_};
        return size_;
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
};

template <typename... Args_>
using event = basic_event<spinlock, Args_...>;

template <typename... Args_>
using event_st = basic_event<null_mutex, Args_...>;

}  // namespace cpph
