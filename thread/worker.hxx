// MIT License
//
// Copyright (c) 2021-2022. Seungwoo Kang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// project home: https://github.com/perfkitpp

#pragma once
#include <atomic>
#include <thread>

#include "../macros.hxx"

//
#include "../__namespace__"

namespace CPPHEADERS_NS_::thread {
class worker
{
   public:
    template <typename Starter_, typename Fn_, typename... Args_>
    void repeat_with_start(Starter_&& starter, Fn_&& loop_fn, Args_&&... args) noexcept
    {
        shutdown();
        _active.store(true);
        _worker = std::thread{
                [this](Starter_&& starter, Fn_&& fn, Args_&&... inner_args) {
                    starter();
                    while (_active)
                    {
                        fn(std::forward<Args_>(inner_args)...);
                    }
                },
                std::forward<Starter_>(starter),
                std::forward<Fn_>(loop_fn),
                std::forward<Args_>(args)...};
    }

    template <typename Fn_, typename... Args_>
    void repeat(Fn_&& loop_fn, Args_&&... args) noexcept
    {
        repeat_with_start([] {}, std::forward<Fn_>(loop_fn), std::forward<Args_>(args)...);
    }

    template <typename Starter_, typename... Args_>
    void start(Starter_&& fn, Args_&&... args)
    {
        shutdown();
        _active.store(true);
        _worker = std::thread{std::forward<Starter_>(fn), std::forward<Args_>(args)..., &_active};
    }

    void stop()
    {
        _active.store(false);
    }

    void join()
    {
        if (_worker.get_id() != std::this_thread::get_id())
            _worker.joinable() && (_worker.join(), 0);
    }

    void shutdown() noexcept
    {
        stop();
        join();
    }

    bool active() const noexcept
    {
        return _active;
    }

    ~worker() noexcept
    {
        shutdown();
    }

   private:
    std::thread _worker;
    std::atomic_bool _active;
};

}  // namespace CPPHEADERS_NS_::thread