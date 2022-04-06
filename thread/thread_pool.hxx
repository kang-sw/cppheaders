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
#include <cassert>
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
    enum class worker_state {
        idle,
        copying,
        invoke,
        expire,
    };

    struct worker_node {
        std::atomic<worker_state> state = worker_state::idle;
        std::thread               thread;
        thread::event_wait        event_wait;

        function<void()>          next;
    };

    using worker_container_type = std::vector<std::unique_ptr<worker_node>>;

   private:
    worker_container_type _threads;
    std::atomic<size_t>   _round = 0;
    std::atomic<size_t>   _on_use = 0;

   private:
    void _worker_function(worker_node* self)
    {
        for (;;) {
            switch (self->state.load(std::memory_order_acquire)) {
                case worker_state::idle:
                case worker_state::copying:
                    break;

                case worker_state::invoke: {
                    std::exchange(self->next, {})();

                    worker_state state_shouldbe = worker_state::invoke;
                    self->state.compare_exchange_strong(state_shouldbe, worker_state::idle);

                    _on_use.fetch_add(-1);
                    break;
                }

                case worker_state::expire:
                    return;

                default:
                    assert(false);
                    break;
            }

            self->event_wait.wait();
        }
    }

   public:
    explicit thread_pool(size_t num_threads = std::thread::hardware_concurrency())
    {
        _threads.reserve(num_threads);

        while (num_threads--) {
            auto& node = _threads.emplace_back(std::make_unique<worker_node>());
            node->thread = std::thread(&thread_pool::_worker_function, this, node.get());
        }
    }

    ~thread_pool()
    {
        for (auto& t : _threads) { t->state.store(worker_state::expire, std::memory_order_release); }
        for (auto& t : _threads) { t->event_wait.notify_one(); }
        for (auto& t : _threads) { t->thread.join(); }
    }

    template <typename Message_>
    void post(Message_&& msg)
    {
        // Wait if thread pool is too busy
        while (_threads.size() == _on_use.load())
            std::this_thread::sleep_for(std::chrono::microseconds{1});

        for (;;) {
            auto round = _round.fetch_add(1);

            auto node = _threads[round % _threads.size()].get();
            auto current_state = worker_state::idle;

            if (node->state.compare_exchange_strong(current_state, worker_state::copying)) {
                _on_use.fetch_add(1);

                node->next = std::forward<Message_>(msg);
                node->state.store(worker_state::invoke, std::memory_order_release);

                node->event_wait.notify_one();
                break;
            } else if (current_state == worker_state::expire) {
                throw std::logic_error{"Call to post() after expiration!"};
            } else {
                std::this_thread::yield();  // Just wait
            }
        }
    }
};
}  // namespace CPPHEADERS_NS_
