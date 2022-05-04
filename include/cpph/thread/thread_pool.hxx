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
#include <atomic>
#include <thread>
#include <vector>

#include "../counter.hxx"
#include "event_queue.hxx"

namespace cpph {
class thread_pool
{
    event_queue _proc;
    std::vector<std::thread> _workers;

   public:
    explicit thread_pool(
            size_t num_threads = std::thread::hardware_concurrency(),
            size_t allocator_memory = 0)
            : _proc(allocator_memory ? allocator_memory : (10 << 10))
    {
        _workers.reserve(num_threads);
        for (auto _ : count(num_threads)) { _workers.emplace_back(&event_queue::exec, &_proc); }
    }

    ~thread_pool()
    {
        stop();
        join();
    }

    void stop()
    {
        _proc.stop();
    }

    void join()
    {
        for (auto& th : _workers) { th.join(); }

        _proc.clear();
        _workers.clear();
    }

    template <typename Message_>
    void post(Message_&& msg)
    {
        _proc.post(std::forward<Message_>(msg));
    }

    template <typename Message_>
    void defer(Message_&& msg)
    {
        _proc.defer(std::forward<Message_>(msg));
    }

    template <typename Message_>
    void dispatch(Message_&& msg)
    {
        _proc.dispatch(std::forward<Message_>(msg));
    }

    auto queue()
    {
        return &_proc;
    }
};

namespace thread {
struct lazy_t {};
constexpr lazy_t lazy;
}  // namespace thread

class event_queue_worker
{
    event_queue _proc;
    std::thread _worker;

   public:
    explicit event_queue_worker(size_t allocator_memory)
            : _proc(allocator_memory),
              _worker(&event_queue::exec, &_proc)
    {
    }

    explicit event_queue_worker(thread::lazy_t, size_t allocator_memory)
            : _proc(allocator_memory)
    {
    }

    event_queue_worker()
            : event_queue_worker(10 << 10)
    {
    }

    ~event_queue_worker()
    {
        shutdown();
    }

   public:
    void stop()
    {
        _proc.stop();
    }

    void join()
    {
        if (_worker.joinable()) { _worker.join(); }
    }

    void launch()
    {
        if (_worker.joinable()) { throw std::logic_error{"You may not relaunch running thread!"}; }
        _proc.restart();
        _worker = std::thread{&event_queue::exec, &_proc};
    }

    void shutdown()
    {
        stop();
        join();
    }

    void clear()
    {
        _proc.clear();
    }

   public:
    auto& queue() { return _proc; }

    template <typename Message_>
    void post(Message_&& msg)
    {
        _proc.post(std::forward<Message_>(msg));
    }

    template <typename Message_>
    void defer(Message_&& msg)
    {
        _proc.defer(std::forward<Message_>(msg));
    }

    template <typename Message_>
    void dispatch(Message_&& msg)
    {
        _proc.dispatch(std::forward<Message_>(msg));
    }
};
}  // namespace cpph