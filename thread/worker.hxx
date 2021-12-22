#pragma once
#include <atomic>
#include <thread>

#include "../macros.hxx"

//
#include "../__namespace__.h"

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