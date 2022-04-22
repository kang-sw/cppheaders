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

#include "../__namespace__"
#include "../circular_queue.hxx"
#include "../memory/queue_allocator.hxx"
#include "../template_utils.hxx"
#include "../thread/event_wait.hxx"
#include "../thread/locked.hxx"

namespace CPPHEADERS_NS_ {
/**
 *
 */
class message_procedure
{
   private:
    struct callable_pair {
        void (*fn)(void*);
        void* body;
    };

   private:
    locked<queue_allocator>               _queue_alloc;
    locked<circular_queue<callable_pair>> _messages;
    std::atomic_size_t                    _num_active_runner = 0;

   public:
    explicit message_procedure(size_t num_max_message, size_t num_queue_buffer)
            : _queue_alloc(num_queue_buffer), _messages(num_max_message) {}

   public:
    size_t run()
    {
    }

    template <typename Duration>
    size_t run_for(Duration&& dur)
    {
    }

    template <typename Timestamp>
    size_t run_until(Timestamp&& until)
    {
    }

    void run_once()
    {
        callable_pair msg = {};
    }

    template <typename Message, typename = enable_if_t<is_invocable_v<Message>>>
    void post(Message&& message)
    {
        bool okay = false;

        do {
            try {
                auto constexpr fn = [](void* ptr) { (*(Message*)ptr)(); };
                void* ptr = _queue_alloc.lock()->template construct<Message>(std::move(message));
                auto  msg = callable_pair{fn, ptr};

                do {
                    _messages.access([&msg, &okay](decltype(_messages)::value_type& v) {
                        if (not v.is_full()) {
                            v.push_back(msg);
                            okay = true;
                        } else {
                            std::this_thread::yield();
                        }
                    });
                } while (not okay);
            } catch (queue_out_of_memory&) {
                std::this_thread::yield();
                continue;  // Just do it again until ready.
            }
        } while (not okay);
    }
};
}  // namespace CPPHEADERS_NS_
