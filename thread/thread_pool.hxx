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
    struct worker_context_t {
        std::thread        thread;
        thread::event_wait event;
        std::atomic_bool   awake = false;
    };

    using worker_container_type = std::vector<std::unique_ptr<worker_context_t>>;
    using task_type = function<void()>;
    using task_queue_type = std::list<task_type>;

    enum class dispatch_state {
        sleeping,
        ready,
        waiting,
    };

   private:
    struct {
        std::thread                 thread;
        thread::event_wait          event;
        locked<task_queue_type>     commits;
        std::atomic<dispatch_state> state = dispatch_state::sleeping;
    } _dispatch;

    locked<task_queue_type> _pending_invoke;
    std::atomic_bool        _pending_invoke_ready = false;

    // Make cache line remain untouched as long as possible
    alignas(64) worker_container_type _workers;
    alignas(64) std::atomic_bool _close = false;

   private:
    void _worker_fn(worker_context_t* self)
    {
        task_queue_type fetched;

        while (not _close.load(std::memory_order_acquire)) {
            if (_pending_invoke_ready.load(std::memory_order_acquire)) {
                // Retrive single callback from task queue, and invoke.
                _pending_invoke.access([&](auto&& list) {
                    if (list.empty()) { return; }
                    fetched.splice(fetched.begin(), list, list.begin());
                    _pending_invoke_ready.store(not list.empty(), std::memory_order_release);
                });

                // It can fail if multiple threads access on same flag
                if (not fetched.empty()) {
                    assert(fetched.size() == 1);

                    fetched.front()();
                    fetched.clear();
                }

                continue;
            }

            // If this should be kept awaken, just yield once.
            if (self->awake.load(std::memory_order_acquire)) {
                std::this_thread::yield();
                continue;
            }

            // Sleep until next awake
            self->event.wait([&] {
                return self->awake.load(std::memory_order_acquire)
                    || _pending_invoke_ready.load(std::memory_order_acquire);
            });
        }
    }

    void _dispatch_fn()
    {
        using std::chrono::steady_clock;
        using namespace std::literals;

        auto            self = &_dispatch;
        task_queue_type bucket;

        auto            now = steady_clock::now();
        auto            until = now;

        size_t          num_awake = 0;
        auto const      num_workers = _workers.size();

        // Primary loop
        while (not _close.load(std::memory_order_acquire)) {
            now = steady_clock::now();

            // Check for input buffer
            switch (self->state.exchange(dispatch_state::waiting)) {
                case dispatch_state::ready: {
                    until = now + 200us;  // Refresh sleep timeout

                    // Retrieve queued tasks
                    self->commits.access([&](auto&& list) {
                        bucket.splice(bucket.begin(), list);
                    });

                    size_t ntask = 0;

                    // Copy to actual invoke queue
                    _pending_invoke.access([&](auto&& list) {
                        list.splice(list.end(), bucket);
                        ntask = list.size();

                        // Refresh ready state
                        _pending_invoke_ready.store(true, std::memory_order_release);
                    });

                    // Clamp task count to number of maximum concurrences ...
                    ntask = std::min(ntask, num_workers);

                    // Sleep overly awoken workers
                    if (ntask < num_awake) {
                        for (auto i : count(ntask, num_awake)) {
                            auto& work = _workers[i];
                            work->awake.store(false, std::memory_order_release);
                        }

                        num_awake = ntask;
                    }

                    // Wakeup
                    for (; num_awake < ntask; ++num_awake) {
                        auto& work = _workers[num_awake];
                        work->event.notify_one([&] {
                            work->awake.store(true, std::memory_order_release);
                        });
                    }

                    continue;
                }

                case dispatch_state::sleeping:
                    break;

                case dispatch_state::waiting: {
                    if (now < until) {
                        std::this_thread::yield();
                        continue;
                    }

                    break;
                }

                default:
                    abort();
            }

            // Mark all workers sleeping
            for (auto i : counter(num_awake)) {
                auto& work = _workers[i];
                work->awake = false;
            }

            num_awake = 0;

            // Turn to sleeping state
            self->event.wait_pp(
                    [&] {
                        // As other thread may switch state to ready between this gap, use CAS.
                        auto state = dispatch_state::waiting;
                        self->state.compare_exchange_strong(state, dispatch_state::sleeping);
                    },
                    [&] {
                        return self->state != dispatch_state::sleeping;
                    });
        }

        // Cleanup worker threads
        for (auto& t : _workers) { t->awake = true; }
        for (auto& t : _workers) { t->event.notify_one(); }
        for (auto& t : _workers) { t->thread.join(); }
    }

   public:
    explicit thread_pool(size_t num_threads = std::thread::hardware_concurrency())
    {
        _workers.reserve(num_threads);
        for (auto i : counter(num_threads)) {
            auto& pref = _workers.emplace_back();
            pref = std::make_unique<worker_context_t>();

            pref->thread = std::thread{&thread_pool::_worker_fn, this, pref.get()};
        }

        _dispatch.thread = std::thread{&thread_pool::_dispatch_fn, this};
    }

    ~thread_pool()
    {
        _dispatch.event.notify_one([&] {
            _dispatch.state = dispatch_state::waiting;
            _close = true;
        });

        _dispatch.thread.join();
    }

    template <typename Message_>
    void post(Message_&& msg)
    {
        assert(not _close.load(std::memory_order_acquire));
        _dispatch.commits.lock()->emplace_back(std::forward<Message_>(msg));

        // Only notify when dispatcher is not sleeping.
        if (_dispatch.state.exchange(dispatch_state::ready) == dispatch_state::sleeping) {
            _dispatch.event.notify_one([&] {
                _dispatch.state.store(dispatch_state::ready, std::memory_order_release);
            });
        }
    }
};
}  // namespace CPPHEADERS_NS_