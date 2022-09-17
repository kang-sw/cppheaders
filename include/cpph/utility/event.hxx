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

inline int operator&(event_control a, event_control b)
{
    return int(a) & int(b);
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

template <class T>
using event_fwd_arg_t = conditional_t<std::is_reference_v<T>, T, T const&>;

struct if_event_entity {
    virtual ~if_event_entity() = default;
    virtual void detach() noexcept = 0;
};

class event_handle
{
    template <class, class...>
    friend class basic_event;

    weak_ptr<if_event_entity> node_ = {};

    explicit event_handle(weak_ptr<if_event_entity> p_node) noexcept
            : node_{p_node}
    {
    }

   public:
    event_handle() noexcept = default;
    event_handle(event_handle&& other) noexcept : node_(move(other.node_)) {}
    auto& operator=(event_handle&& o) noexcept { return swap(node_, o.node_), *this; }

    bool valid() noexcept { return not node_.expired(); }
    void expire() noexcept
    {
        if (auto p_node = node_.lock()) { p_node->detach(); }
    }
};

template <typename Mutex, typename... Args>
class basic_event
{
   public:
    using event_fn = ufunction<event_control(Args...)>;
    using handle = event_handle;

    template <class Callable>
    static constexpr inline bool is_event_function_v = is_invocable_v<Callable> || is_invocable_v<Callable, Args...>;

    template <class Callable>
    using enable_if_event_t = enable_if_t<is_event_function_v<Callable>>;

   private:
    struct if_entity : if_event_entity, enable_shared_from_this<if_entity> {
       private:
        atomic<intptr_t> refcnt_ = 1;

       public:
        basic_event* owner = nullptr;
        uint64_t priority = 0;

        weak_ptr<if_entity> p_prev = {};
        shared_ptr<if_entity> p_next = nullptr;

       public:
        virtual event_control operator()(event_fwd_arg_t<Args>...) = 0;

        virtual ~if_entity() override
        {
            scoped_lock _{owner->mtx_};
            --owner->size_;
        }

        void detach() noexcept override
        {
            scoped_lock _{owner->mtx_};

            // Validate
            if (ptr_equals(owner->new_node_front_, this->weak_from_this()))
                owner->new_node_front_ = p_next;
            else if (ptr_equals(owner->node_front_, this->weak_from_this()))
                owner->node_front_ = p_next;

            // Detach this from previous / next node.
            auto next = exchange(p_next, {});
            auto prev = exchange(p_prev, {}).lock();

            if (next) { next->p_prev = prev; }
            if (prev) { prev->p_next = next; }
        }
    };

   public:
    using mutex_type = Mutex;

   private:
    mutable Mutex mtx_;
    size_t size_ = 0;
    shared_ptr<if_entity> node_front_ = nullptr;
    shared_ptr<if_entity> new_node_front_ = nullptr;

   public:
    template <class Callable, class = enable_if_event_t<Callable>>
    static auto weak(weak_ptr<void> wptr, Callable&& callable)
    {
        return [wptr = std::move(wptr),
                callable = std::forward<Callable>(callable)]  //
               (event_fwd_arg_t<Args>... args) mutable
               -> event_control {
                   auto anchor = wptr.lock();  // Prevent anchor to be destroyed during function call
                   if (anchor == nullptr) { return event_control::expire; }

                   if constexpr (std::is_invocable_r_v<event_control, Callable, Args...>)
                       return callable(args...);
                   else if constexpr (std::is_invocable_v<Callable, Args...>)
                       return callable(args...), event_control::ok;
                   else if constexpr (std::is_invocable_r_v<event_control, Callable>)
                       return callable();
                   else if constexpr (std::is_invocable_v<Callable>)
                       return callable(), event_control::ok;
                   else
                       return ((args.INVALID), ...);
               };
    }

   public:
    struct append_proxy {
        basic_event& owner;
        uint64_t priority;

        weak_ptr<void> w_anchor;
        handle* p_store_handle = nullptr;

        template <typename Callable, class = enable_if_event_t<Callable>>
        append_proxy& operator<<(Callable&& fn) noexcept
        {
            handle h_result;
            if (w_anchor.expired())
                h_result = owner.append(std::forward<Callable>(fn), priority);
            else
                h_result = owner.append(weak(w_anchor, std::forward<Callable>(fn)), priority);

            if (auto p_dst = exchange(p_store_handle, nullptr)) { *p_dst = move(h_result); }
            return *this;
        }

        append_proxy& operator<<(handle* dst) noexcept
        {
            p_store_handle = dst;
            return *this;
        }

        append_proxy& operator<<(nullptr_t) noexcept
        {
            w_anchor = {};
            return *this;
        }

        append_proxy& operator<<(weak_ptr<void> wp) noexcept
        {
            w_anchor = move(wp);
            return *this;
        }
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

    append_proxy operator()(event_priority priority, int64_t value) noexcept { return append_proxy{*this, (uint64_t)priority + value}; }
    append_proxy operator()(event_priority priority) noexcept { return append_proxy{*this, (uint64_t)priority}; }
    append_proxy operator()(int64_t value) noexcept { return append_proxy{*this, (uint64_t)event_priority::middle + value}; }
    append_proxy operator()() noexcept { return append_proxy{*this, (uint64_t)event_priority::middle + 0}; }

    class frontend
    {
        basic_event* owner_;

       public:
        explicit frontend(basic_event* owner = nullptr) : owner_(owner) {}

       public:
        auto operator()(event_priority priority, int64_t value) const noexcept { return append_proxy{*owner_, (uint64_t)priority + value}; }
        auto operator()(event_priority priority) const noexcept { return append_proxy{*owner_, (uint64_t)priority}; }
        auto operator()(int64_t value) const noexcept { return append_proxy{*owner_, (uint64_t)event_priority::middle + value}; }
        auto operator()() const noexcept { return append_proxy{*owner_, (uint64_t)event_priority::middle + 0}; }
    };

    frontend make_frontend() noexcept
    {
        return frontend{this};
    }

    void invoke(event_fwd_arg_t<Args>... args)
    {
        // Perform insertion
        this->flush();

        // Invoke chained methods
        mtx_.lock();
        shared_ptr<if_entity> p_head = node_front_;

        while (p_head != nullptr) {
            mtx_.unlock();
            auto p_node = exchange(p_head, p_head->p_next);
            event_control r_invoke = (*p_node)(args...);
            if (r_invoke & event_control::expire) { p_node->detach(), p_node.reset(); }

            mtx_.lock();
            if (r_invoke & event_control::consume) { break; }
        }

        mtx_.unlock();
    }

    void flush()
    {
        scoped_lock _{mtx_};

        while (new_node_front_ != nullptr) {
            auto new_node = exchange(new_node_front_, new_node_front_->p_next);
            new_node->p_prev = {};
            new_node->p_next = nullptr;

            if (node_front_ != nullptr) {
                auto p_insert = node_front_.get();
                while (p_insert && p_insert->priority > new_node->priority) {
                    if (p_insert->p_next != nullptr) {
                        p_insert = p_insert->p_next.get();
                    } else {
                        p_insert->p_next = new_node;
                        new_node->p_prev = p_insert->weak_from_this();

                        // done.
                        p_insert = nullptr;
                    }
                }

                if (p_insert) {
                    if (auto& n = p_insert->p_next) { n->p_prev = new_node; }
                    new_node->p_next = p_insert->p_next;
                    p_insert->p_next = new_node;
                    new_node->p_prev = p_insert->weak_from_this();
                }
            } else {
                node_front_ = move(new_node);
            }
        }
    }

    template <typename Callable, class = enable_if_event_t<Callable>>
    handle append(Callable&& fn, uint64_t priority)
    {
        struct entity_t : if_entity {
            decay_t<Callable> fn_;
            explicit entity_t(Callable&& fn) : fn_(std::forward<Callable>(fn)) {}

            event_control operator()(event_fwd_arg_t<Args>... args) override
            {
                if constexpr (is_invocable_r_v<event_control, Callable, Args...>) {
                    return fn_(args...);
                } else if constexpr (is_invocable_r_v<bool, Callable, Args...>) {
                    return fn_(args...) ? event_control::ok : event_control::expire;
                } else if constexpr (is_invocable_v<Callable, Args...>) {
                    return fn_(args...), event_control::ok;
                } else if constexpr (is_invocable_v<Callable>) {
                    return fn_(), event_control::ok;
                } else {
                    Callable::ERROR_NO_INVOCATION_AVAILABLE;
                }
            }
        };

        shared_ptr<if_entity> p_new_node = make_shared<entity_t>(std::forward<Callable>(fn));
        p_new_node->owner = this;
        p_new_node->priority = priority;

        {
            scoped_lock _{mtx_};
            auto prev_front = exchange(new_node_front_, p_new_node);
            if (prev_front) { prev_front->p_prev = p_new_node; }
            p_new_node->p_next = prev_front;

            ++size_;
        }
        return handle{p_new_node};
    }

    template <typename Callable, class = enable_if_event_t<Callable>>
    handle add(Callable&& fn, event_priority prio, int64_t value)
    {
        return this->append(std::forward<Callable>(fn), uint64_t(prio) + value);
    }

    template <typename Callable, class = enable_if_event_t<Callable>>
    handle add(Callable&& fn, event_priority evt)
    {
        return this->add(std::forward<Callable>(fn), evt, 0);
    }

    template <typename Callable, class = enable_if_event_t<Callable>>
    handle add(Callable&& fn, int64_t value)
    {
        return this->add(std::forward<Callable>(fn), event_priority::middle, value);
    }

    template <typename Callable, class = enable_if_event_t<Callable>>
    handle add(Callable&& fn)
    {
        return this->add(std::forward<Callable>(fn), event_priority::middle, 0);
    }

    template <typename Callable, class = enable_if_event_t<Callable>>
    handle add_weak(weak_ptr<void> wptr, Callable&& callable, event_priority priority = event_priority::middle, int64_t value = 0)
    {
        return this->add(
                weak(move(wptr), std::forward<Callable>(callable)),
                priority,
                value);
    }

    template <typename Callable, class = enable_if_event_t<Callable>>
    handle add(weak_ptr<void> wptr, Callable&& callable, event_priority priority = event_priority::middle, int64_t value = 0)
    {
        return this->add(
                weak(move(wptr), std::forward<Callable>(callable)),
                priority,
                value);
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
