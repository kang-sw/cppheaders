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
#include <thread>

#include "cpph/circular_queue.hxx"
#include "cpph/memory/queue_allocator.hxx"
#include "cpph/template_utils.hxx"
#include "cpph/thread/event_wait.hxx"
#include "cpph/thread/locked.hxx"

namespace cpph {
/**
 *
 */
class message_procedure
{
   private:
    struct callable_pair {
        void (*fn)(void*);
        void* body;
        void* alloc_new;  // Head allocated due to lack of memory
    };

   private:
    locked<queue_allocator> _queue_alloc;
    locked<circular_queue<callable_pair>> _messages;
    thread::event_wait _event_wait;
    std::atomic_bool _stopped = false;
    std::atomic_size_t _num_jobs = 0;
    volatile size_t _num_waiting_runner = 0;

   public:
    explicit message_procedure(size_t num_max_message, size_t num_queue_buffer)
            : _queue_alloc(num_queue_buffer), _messages(num_max_message) {}

   private:
    template <typename WaitFn>
    bool _run_one_impl(WaitFn&& wait)
    {
        callable_pair msg = {};

        // Do quick check
        _messages.access([&](decltype(_messages)::value_type& v) {
            if (v.empty()) {
                ++_num_waiting_runner;
            } else {
                _num_jobs.fetch_sub(1);
                msg = v.dequeue();
            }
        });

        for (; not msg.fn;) {
            // Function couldn't be found. Wait for next post.
            // semaphore must be decreased.
            if (not wait()) {
                _messages.access([&](auto&&) { --_num_waiting_runner; });
                return false;
            }

            // If wait succeeds, try access messages queue again.
            _messages.access([&](decltype(_messages)::value_type& v) {
                if (not v.empty()) {
                    --_num_waiting_runner;

                    _num_jobs.fetch_sub(1);
                    msg = v.dequeue();
                }
            });
        }

        // Invoke and exit.
        msg.fn(msg.body);
        return true;
    }

   public:
    size_t run()
    {
        size_t num_ran = 0;
        for (; run_one(); ++num_ran) {}
        return num_ran;
    }

    template <typename Timestamp>
    size_t run_until(Timestamp&& until)
    {
    }

    template <typename Duration>
    size_t run_for(Duration&& dur)
    {
    }

    void stop()
    {
        release(_stopped, true);
    }

    void restart()
    {
        release(_stopped, false);
    }

    bool run_one()
    {
        return _run_one_impl([&] {
            bool stopped = false;
            _event_wait.wait([&] { return (acquire(_num_jobs) != 0 || (stopped = acquire(_stopped))); });
            return not stopped;
        });
    }

    template <typename Message, typename = enable_if_t<is_invocable_v<Message>>>
    void post(Message&& message)
    {
        auto constexpr fn =
                [](callable_pair* arg) {
                    (*(Message*)arg->fn)();
                    if (arg->alloc_new) { delete (Message*)arg->alloc_new; }
                };

        callable_pair msg = {fn, nullptr, nullptr};

        try {
            msg.body = _queue_alloc.lock()->template construct<Message>(std::forward<Message>(message));
        } catch (queue_out_of_memory&) {
            msg.alloc_new = msg.body = new Message(std::forward<Message>(message));
        }

        bool should_wakeup_any = false;
        _messages.access([&](decltype(_messages)::value_type& v) {
            if (v.is_full())
                v.reserve_shrink(v.capacity() * 2);

            v.push(msg);
            _num_jobs.fetch_add(1);
            should_wakeup_any = (_num_waiting_runner != 0);
        });

        if (should_wakeup_any)
            _event_wait.notify_one();
    }
};
}  // namespace cpph
