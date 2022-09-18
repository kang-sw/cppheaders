/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2022. Seungwoo Kang
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
#include <cpph/std/chrono>
#include <cpph/std/list>
#include <cpph/std/vector>
#include <thread>

#include "cpph/memory/ring_allocator.hxx"
#include "cpph/thread/event_wait.hxx"
#include "cpph/thread/locked.hxx"
#include "cpph/thread/threading.hxx"
#include "cpph/utility/cleanup.hxx"
#include "cpph/utility/functional.hxx"
#include "cpph/utility/generic.hxx"
#include "cpph/utility/templates.hxx"

namespace cpph {

template <class... Args>
class basic_event_queue;

using event_queue = basic_event_queue<>;

/**
 *
 */
template <class... Args>
class basic_event_queue
{
    struct function_node {
        bool is_ring_allocated_;
        function_node* next_;
        void (*disposer_)(function_node*);
        void (*invoke_)(function_node*, add_reference_t<Args>...);
        char data[];

        void operator()(add_reference_t<Args>... args) noexcept { invoke_(this, args...); }
        ~function_node() noexcept { disposer_(this); }
    };

   private:
    mutable spinlock alloc_lock_;
    mutable spinlock msg_lock_;

    ring_allocator alloc_;
    thread::event_wait ewait_;
    atomic_bool stopped_ = false;
    function_node* front_ = nullptr;
    function_node* back_ = nullptr;

   public:
    /**
     * Creates new message procedure.
     *
     * @param queue_buffer_size
     *    Queue buffer size. As it does not resized, set this value big enough on construction.
     *
     * @param num_initial_message_queue_size
     *    Number of initial image queue size. As this circular queue resizes on demand,
     *     determining this value is less important than setting \c num_queue_buffer correctly.
     */
    explicit basic_event_queue(size_t queue_buffer_size)
            : alloc_(queue_buffer_size) {}

    /**
     * Destruct this message procedure
     */
    ~basic_event_queue() { clear(); }

   private:
    static basic_event_queue*& _p_active_exec() noexcept
    {
        static thread_local basic_event_queue* p_exec = nullptr;
        return p_exec;
    }

    template <class RetrFn, class = enable_if_t<is_invocable_r_v<function_node*, RetrFn>>>
    bool _exec_single(RetrFn&& retreive_event, add_reference_t<Args>... args)
    {
        if (function_node* p_func = retreive_event()) {
            // Using cleanup, all states would remain valid on exceptional situation.
            [[maybe_unused]] auto _0 = cleanup(
                    [&, previous = exchange(_p_active_exec(), this)] {
                        _p_active_exec() = previous;  // Rollback active executor as previous one
                        _release(p_func);             // Release allocated function
                    });

            (*p_func)(args...);
            assert(_p_active_exec() == this);
            return true;
        } else {
            return false;
        }
    }

    function_node* _pop_event() noexcept
    {
        lock_guard _{msg_lock_};

        if (not front_) { return nullptr; }
        auto p_func = exchange(front_, front_->next_);
        if (p_func == back_)
            back_ = nullptr;

        return p_func;
    }

    void _push_event(function_node* p_func) noexcept
    {
        lock_guard _{msg_lock_};

        if (auto prev_back = exchange(back_, p_func)) {
            prev_back->next_ = p_func;
        }

        if (front_ == nullptr)
            front_ = p_func;
    }

    void _release(function_node* p_func) noexcept
    {
        (*p_func).~function_node();

        if (p_func->is_ring_allocated_) {
            lock_guard _{alloc_lock_};
            alloc_.deallocate(p_func);
        } else {
            delete[] (char*)p_func;
        }
    }

   public:
    bool empty() const
    {
        return lock_guard{msg_lock_}, front_ == nullptr;
    }

    bool exec_one(add_reference_t<Args>... args)
    {
        return _exec_single(
                [this] {
                    function_node* p_func = nullptr;
                    ewait_.wait([&] { return acquire(stopped_) || (p_func = _pop_event()); });
                    return p_func;
                },
                args...);
    }

    template <typename Clock, typename Duration>
    bool exec_one_until(add_reference_t<Args>... args, time_point<Clock, Duration> const& until)
    {
        return _exec_single(
                [&] {
                    function_node* p_func = nullptr;
                    ewait_.wait_until(until, [&] { return acquire(stopped_) || (p_func = _pop_event()); });
                    return p_func;
                },
                args...);
    }

    template <typename Rep, typename Period>
    bool exec_one_for(add_reference_t<Args>... args, duration<Rep, Period> const& dur)
    {
        return exec_one_until(args..., steady_clock::now() + dur);
    }

    size_t exec(add_reference_t<Args>... args)
    {
        size_t nexec = 0;
        for (; not acquire(stopped_) && exec_one(args...); ++nexec) {}
        return nexec;
    }

    size_t flush(add_reference_t<Args>... args)
    {
        size_t num_ran = 0;
        auto exec_fn = [&] {
            function_node* p_func = nullptr;
            ewait_.wait([&] { return acquire(stopped_) || (p_func = _pop_event(), true); });
            return p_func;
        };

        for (; not acquire(stopped_) && _exec_single(exec_fn, args...); ++num_ran) {}
        return num_ran;
    }

    template <typename Clock, typename Duration>
    size_t run_until(add_reference_t<Args>... args, time_point<Clock, Duration> const& until)
    {
        size_t num_ran = 0;
        for (; not acquire(stopped_) && exec_one_until(args..., until); ++num_ran) {}
        return num_ran;
    }

    template <typename Rep, typename Period>
    size_t run_for(add_reference_t<Args>... args, duration<Rep, Period> const& dur)
    {
        size_t num_ran = 0;
        auto until = std::chrono::steady_clock::now() + dur;

        for (; not acquire(stopped_) && exec_one_until(args..., until); ++num_ran) {}
        return num_ran;
    }

    void stop()
    {
        ewait_.notify_all([&] { release(stopped_, true); });
    }

    void restart()
    {
        release(stopped_, false);
    }

    void clear() noexcept
    {
        // Retrieve functions
        msg_lock_.lock();
        auto p_head = exchange(front_, nullptr);
        back_ = nullptr;
        msg_lock_.unlock();

        while (p_head != nullptr) {
            _release(exchange(p_head, p_head->next_));
        }
    }

   public:
    template <typename Message, typename = enable_if_t<is_invocable_v<Message, Args...>>>
    void post(Message&& message)
    {
        function_node* p_func;
        {
            lock_guard _{alloc_lock_};
            p_func = (function_node*)alloc_.allocate_nt(sizeof(function_node) + sizeof(Message));
        }

        if (p_func != nullptr) {
            p_func->is_ring_allocated_ = true;
        } else {
            p_func = (function_node*)new char[sizeof(function_node) + sizeof(Message)];
            p_func->is_ring_allocated_ = false;
        }

        auto p_msg = new (p_func->data) Message{std::forward<Message>(message)};

        p_func->next_ = nullptr;
        p_func->disposer_ = [](function_node* p) { ((Message&)p->data).~Message(); };
        p_func->invoke_ = [](function_node* p, add_reference_t<Args>... args) { move((Message&)p->data)(args...); };

        ewait_.notify_one([&] { _push_event(p_func); });
    }

    template <typename Message, typename = enable_if_t<is_invocable_v<Message, Args...>>>
    void dispatch(Message&& message)
    {
        if (_p_active_exec() == this) {
            forward<Message>(message)();
        } else {
            post(forward<Message>(message));
        }
    }

   public:
    struct payload_deallocator {
        basic_event_queue* p_owner_;
        void operator()(char* p) noexcept
        {
            if (p_owner_) {
                lock_guard _{p_owner_->alloc_lock_};
                p_owner_->alloc_.deallocate(p);
            } else {
                delete[] p;
            }
        }
    };

    using temporary_payload_ptr = ptr<char[], payload_deallocator>;

    //! Do not use this except for temporary post data generation!
    auto allocate_temporary_payload(size_t nbyte) noexcept -> temporary_payload_ptr
    {
        temporary_payload_ptr ptr;
        {
            lock_guard _{alloc_lock_};
            ptr = temporary_payload_ptr((char*)alloc_.allocate_nt(nbyte), payload_deallocator{this});
        }

        if (ptr == nullptr) {
            ptr = temporary_payload_ptr{new char[nbyte], payload_deallocator{nullptr}};
        }

        return ptr;
    }
};

template <class Func, class... Args>
struct event_queue_binder {
    basic_event_queue<Args...>& ioc_;
    Func callable_;

    template <class... Params>
    void operator()(Params&&... params) const
    {
        ioc_.post(bind_front(callable_, std::forward<Params>(params)...));
    }
};

template <class Fn, class... Args, class... Params>
auto bind_event_queue(basic_event_queue<Params...>& ioc, Fn&& fn, Args&&... args)
{
    auto func = share_callable(bind_front(std::forward<Fn>(fn), std::forward<Args>(args)...));
    return event_queue_binder<decltype(func), Params...>{ioc, std::move(func)};
}

template <class Fn, class... Args, class... Params>
auto bind_event_queue_weak(basic_event_queue<Params...>& ioc, weak_ptr<void> wp, Fn&& fn, Args&&... args)
{
    auto func = share_callable(bind_front_weak(move(wp), std::forward<Fn>(fn), std::forward<Args>(args)...));
    return event_queue_binder<decltype(func), Params...>{ioc, std::move(func)};
}

}  // namespace cpph
