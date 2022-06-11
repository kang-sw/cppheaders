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
#include "cpph/thread/event_wait.hxx"
#include "event_queue.hxx"
#include "utility/functional.hxx"

namespace cpph::thread {
/**
 * Indicates single pipe
 */
template <typename SharedContext, typename InputType, typename EventProc = event_queue>
class pipe_mono
{
    EventProc* const _ioc;
    shared_ptr<SharedContext> _buf_shared = nullptr;

    function<void(shared_ptr<SharedContext>, InputType&)> const _procedure;
    InputType _next_input = {};

    bool _busy = false;

    event_wait _wait;
    shared_ptr<void> _anchor = make_shared<nullptr_t>();

   public:
    template <typename Callable, typename... Args>
    explicit pipe_mono(EventProc* ioc, Callable&& callable, Args&&... args)
            : _ioc(ioc),
              _procedure(bind_front(std::forward<Callable>(callable), std::forward<Args>(args)...))
    {
    }

    ~pipe_mono() noexcept
    {
        _anchor.reset();
    }

   public:
    template <typename CommitFn, typename Timeout>
    bool commit(CommitFn&& fn, shared_ptr<SharedContext> shared, Timeout&& timeout)
    {
        if (not _wait.wait_for(timeout, [&] { return not _busy; }))
            return false;

        // Critical section not required as host callback is already disposed.
        _buf_shared = std::move(shared);
        fn(_next_input);

        // Call next fence
        _busy = true;
        _ioc->post(bind_front_weak(_anchor, &pipe_mono::_fn_invoke, this));
    }

   private:
    void _fn_invoke()
    {
        _procedure(std::move(_buf_shared), _next_input);
        _wait.notify_one([&] { _busy = false; });
    }
};
}  // namespace cpph::thread
