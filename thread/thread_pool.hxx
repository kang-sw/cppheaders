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
#include <list>
#include <thread>
#include <vector>

#include "../__namespace__"
#include "../counter.hxx"
#include "../functional.hxx"
#include "event_wait.hxx"
#include "locked.hxx"

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
    using task_queue_type = locked<std::list<function<void()>>>;

   private:
    worker_container_type _threads;
    std::atomic<size_t>   _round = 0;

    // Quick check without locking queue
    std::atomic<size_t> _num_queued_unsafe = 0;
    task_queue_type     _queued;

   private:
    void _worker_function(worker_node* self)
    {
        for (;;) {
            auto const state = self->state.load(std::memory_order_acquire);
            bool       set_hot = false;

            switch (state) {
                case worker_state::copying:
                    break;

                case worker_state::invoke: {
                    std::exchange(self->next, {})();

                    worker_state state_shouldbe = worker_state::invoke;
                    self->state.compare_exchange_strong(state_shouldbe, worker_state::idle);

                    [[fallthrough]];
                }

                case worker_state::idle: {
                    // This may be awoken for queued event flush request
                    worker_state state_shouldbe = worker_state::idle;

                    // Worker must be in idle state. Otherwise, another request is being copied.
                    if (self->state.compare_exchange_strong(state_shouldbe, worker_state::invoke)) {
                        assert(not self->next);
                        using namespace std::literals;
                        using std::chrono::steady_clock;

                        // Remain hot for 1 millisecond
                        auto now = steady_clock::now();
                        auto hot_until = now + 1ms;

                        for (; now < hot_until; now = steady_clock::now()) {
                            if (_num_queued_unsafe == 0) {
                                std::this_thread::yield();
                                break;
                            }

                            if (auto queue = _queued.lock(); not queue->empty()) {
                                _num_queued_unsafe.fetch_add(-1);
                                self->next = std::move(queue->front());
                                queue->pop_front();
                            } else {
                                std::this_thread::yield();
                                break;
                            }

                            std::exchange(self->next, {})();
                        }

                        // Reset state
                        state_shouldbe = worker_state::invoke;
                        self->state.compare_exchange_strong(state_shouldbe, worker_state::idle);
                    }

                    break;
                }

                case worker_state::expire:
                    return;

                default:
                    abort();
            }

            self->event_wait.wait(
                    [&] {
                        switch (self->state.load()) {
                            case worker_state::idle:
                                return not _queued.lock()->empty();

                            case worker_state::invoke:
                            case worker_state::expire:
                                return true;

                            default:
                                return false;
                        }
                    });
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
        // If all threads are busy, just put it to queue
        size_t       chances = _threads.size();
        worker_node* node = nullptr;

        for (; chances--; _round.fetch_add(1)) {
            size_t round = _round;
            node = _threads[round % _threads.size()].get();

            auto current_state = worker_state::idle;

            if (node->state.compare_exchange_strong(current_state, worker_state::copying)) {
                assert(not node->next);

                node->next = std::forward<Message_>(msg);
                node->state.store(worker_state::invoke);

                node->event_wait.notify_all();
                node = nullptr;
                break;
            } else if (current_state == worker_state::expire) {
                throw std::logic_error{"Call to post() after expiration!"};
            }
        }

        if (node) {
            bool was_empty = false;
            {
                auto queue = _queued.lock();
                was_empty = queue->empty();

                queue->emplace_back(std::forward<Message_>(msg));
            }

            _num_queued_unsafe.fetch_add(1);

            // To prevent 'already-finished-my-work-and-sleeping'
            if (was_empty)
                node->event_wait.notify_all();
        }
    }
};
}  // namespace CPPHEADERS_NS_
