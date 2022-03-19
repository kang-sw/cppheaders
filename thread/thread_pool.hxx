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

//
// Created by ki608 on 2022-03-19.
//

#pragma once
#include <queue>
#include <thread>
#include <vector>

#include "../__namespace__"
#include "../counter.hxx"
#include "../functional.hxx"
#include "event_wait.hxx"

namespace CPPHEADERS_NS_ {
class thread_pool
{
    volatile bool                _is_alive{true};
    std::vector<std::thread>     _threads;

    std::deque<function<void()>> _event_queue;
    thread::event_wait           _event_wait;

   public:
    explicit thread_pool(size_t num_threads = std::thread::hardware_concurrency())
    {
        while (num_threads--)
            _threads.emplace_back(bind_front(&thread_pool::_worker_loop, this));
    }

    ~thread_pool()
    {
        _event_wait.notify_all([&] { _is_alive = false; });
        for (auto& thr : _threads) { thr.join(); }

        _threads.clear();
    }

    template <typename Message_>
    void post(Message_&& msg)
    {
        // TODO: use custom allocator?
        _event_wait.notify_one([&] {
            _event_queue.emplace_back(std::forward<Message_>(msg));
        });
    }

   private:
    void _worker_loop()
    {
        function<void()> fn;

        for (;;) {
            _event_wait.wait([&] {
                if (not _is_alive) { return true; }
                if (_event_queue.empty()) { return false; }

                fn = std::move(_event_queue.front());
                _event_queue.pop_front();
                return true;
            });

            if (not fn) { return; }
            std::exchange(fn, {})();
        }
    }
};
}  // namespace CPPHEADERS_NS_
