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
#include <cpph/std/list>
#include <cpph/std/vector>
#include <thread>

#include "cpph/container/circular_queue.hxx"
#include "cpph/memory/ring_allocator.hxx"
#include "cpph/thread/event_wait.hxx"
#include "cpph/thread/locked.hxx"
#include "cpph/utility/generic.hxx"

namespace cpph {
/**
 *
 */
class event_queue
{
   private:
    struct callable_pair {
        void (*fn)(callable_pair const*);
        void (*dispose)(event_queue*, void*);
        void* body;
    };

   private:
    locked<ring_allocator_with_fb> _queue_alloc;
    locked<circular_queue<callable_pair>> _messages;
    thread::event_wait _event_wait;
    std::atomic_bool _stopped = false;
    volatile size_t _num_waiting_runner = 0;

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
    explicit event_queue(size_t queue_buffer_size, size_t num_initial_message_queue_size = 32)
            : _queue_alloc(queue_buffer_size), _messages(num_initial_message_queue_size) {}

    /**
     * Destruct this message procedure
     */
    ~event_queue() { clear(); }

   private:
    template <typename Message, typename = enable_if_t<is_invocable_v<Message>>>
    callable_pair _allocate_token(Message&& message) noexcept
    {
        auto constexpr fn = [](callable_pair const* arg) { (*(Message*)arg->body)(); };
        callable_pair msg = {fn, nullptr, nullptr};

        msg.body = _queue_alloc.lock()->allocate(sizeof message);
        msg.dispose =
                [](event_queue* self, void* body) {
                    (*(Message*)body).~Message();
                    self->_queue_alloc.lock()->deallocate(body);
                };

        new (msg.body) Message(std::forward<Message>(message));
        return msg;
    }

   private:
    static bool& _deferred_wakeup_flag() noexcept
    {
        thread_local static bool _flag = false;
        return _flag;
    }

    static auto _deferred_list() noexcept
    {
        thread_local static std::vector<callable_pair> _list;
        return &_list;
    }

    static auto _active_executor() -> void const*&
    {
        thread_local static void const* _inst = nullptr;
        return _inst;
    }

    void _invoke(callable_pair const& msg)
    {
        _active_executor() = this;
        msg.fn(&msg);
        msg.dispose(this, msg.body);
        _active_executor() = nullptr;

        // Flush deferred list after invocation
        if (auto deferred = _deferred_list(); not deferred->empty()) {
            bool should_wakeup_other_thread = false;
            _messages.access([&](decltype(_messages)::value_type& e) {
                if (e.capacity() - e.size() < deferred->size())
                    e.reserve_shrink(std::max(e.capacity() + deferred->size(), e.size() * 2));

                auto num_deferred_jobs = deferred->size();
                should_wakeup_other_thread = _num_waiting_runner > 0 && num_deferred_jobs > 1;
                e.enqueue_n(deferred->begin(), num_deferred_jobs);
            });

            // Defer waking up other threads.
            _deferred_wakeup_flag() = should_wakeup_other_thread;
            deferred->clear();
        } else {
            _deferred_wakeup_flag() = false;
        }
    }

    template <typename WaitFn>
    bool _exec_one_impl(WaitFn&& fn_wait)
    {
        callable_pair msg = {};

        // Wakeup other threads to process deferred jobs.
        size_t num_wakeup_count = 0;

        // Do quick check
        _messages.access([&](decltype(_messages)::value_type& v) {
            if (v.empty()) {
                ++_num_waiting_runner;
            } else {
                msg = v.dequeue();
                auto num_jobs = v.size();

                if (exchange(_deferred_wakeup_flag(), false) && num_jobs > 0) {
                    num_wakeup_count = std::min(num_jobs, size_t(_num_waiting_runner));
                }
            }
        });

        while (num_wakeup_count--)
            _event_wait.notify_one();  // wakeup other thread to process

        for (; msg.fn == nullptr;) {
            // Function couldn't be found. Wait for next post.
            // semaphore must be decreased.
            if (not fn_wait()) {
                _messages.access([&](auto&&) { --_num_waiting_runner; });
                return false;
            } else {
                // If wait succeeds, try access messages queue again.
                _messages.access([&](decltype(_messages)::value_type& v) {
                    if (not v.empty()) {
                        --_num_waiting_runner;
                        msg = v.dequeue();
                    }
                });
            }
        }

        // Invoke and exit.
        _invoke(msg);

        return true;
    }

    bool _has_any_job_locked()
    {
        return not _messages.lock()->empty();
    }

   public:
    void touch_one()
    {
        _event_wait.notify_one();
    }

    void touch_all()
    {
        _event_wait.notify_all();
    }

    bool empty() const
    {
        return _messages.lock()->empty();
    }

    bool exec_one()
    {
        return _exec_one_impl([&] {
            bool stopped = false;
            _event_wait.wait([&] { return (_has_any_job_locked() || (stopped = acquire(_stopped))); });
            return not stopped;
        });
    }

    template <typename Timestamp>
    bool exec_one_until(Timestamp&& until)
    {
        return _exec_one_impl([&] {
            bool stopped = false;
            bool timeout = not _event_wait.wait_until(
                    std::forward<Timestamp>(until),
                    [&] {
                        return (_has_any_job_locked() || (stopped = acquire(_stopped)));
                    });

            return not stopped && not timeout;
        });
    }

    template <typename Duration>
    bool exec_one_for(Duration&& dur)
    {
        return exec_one_until(std::chrono::steady_clock::now() + dur);
    }

    size_t exec()
    {
        size_t num_ran = 0;
        for (; not acquire(_stopped) && exec_one(); ++num_ran) {}
        return num_ran;
    }

    size_t flush()
    {
        size_t num_ran = 0;

        for (;;) {
            callable_pair msg = {};

            // Do quick check
            _messages.access([&](decltype(_messages)::value_type& v) {
                if (not v.empty() && not acquire(_stopped)) {
                    msg = v.dequeue();
                }
            });

            if (msg.fn == nullptr)
                break;

            _invoke(msg);
            ++num_ran;
        }

        return num_ran;
    }

    template <typename Timestamp>
    size_t run_until(Timestamp&& until)
    {
        size_t num_ran = 0;
        for (; not acquire(_stopped) && exec_one_until(until); ++num_ran) {}
        return num_ran;
    }

    template <typename Duration>
    size_t run_for(Duration&& dur)
    {
        size_t num_ran = 0;
        auto until = std::chrono::steady_clock::now() + dur;

        for (; not acquire(_stopped) && exec_one_until(until); ++num_ran) {}
        return num_ran;
    }

    void stop()
    {
        _event_wait.notify_all([&] { release(_stopped, true); });
    }

    void restart()
    {
        release(_stopped, false);
    }

    void clear(size_t next_size = 32)
    {
        decltype(_messages)::value_type queue{next_size};
        _messages.access([&](decltype(queue)& v) {
            swap(queue, v);
        });

        for (auto& e : queue) {
            e.dispose(this, e.body);
        }
    }

   public:
    template <typename Message, typename = enable_if_t<is_invocable_v<Message>>>
    void post(Message&& message)
    {
        auto msg = _allocate_token(std::forward<Message>(message));

        bool should_wakeup_any = false;
        _messages.access([&](decltype(_messages)::value_type& v) {
            if (v.is_full())
                v.reserve_shrink(v.capacity() * 2);

            v.push(msg);
            should_wakeup_any = (_num_waiting_runner != 0);
        });

        std::atomic_thread_fence(std::memory_order_release);
        _event_wait.notify_one();
    }

    template <typename Message, typename = enable_if_t<is_invocable_v<Message>>>
    void defer(Message&& message)
    {
        if (this == _active_executor()) {
            auto msg = _allocate_token(std::forward<Message>(message));
            _deferred_list()->emplace_back(msg);
        } else {
            post(std::forward<Message>(message));
        }
    }

    template <typename Message, typename = enable_if_t<is_invocable_v<Message>>>
    void dispatch(Message&& message)
    {
        if (this == _active_executor()) {
            message();
        } else {
            post(std::forward<Message>(message));
        }
    }
};
}  // namespace cpph
