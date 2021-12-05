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
    template <typename Fn_, typename... Args_>
    void repeat(Fn_&& loop_fn, Args_&&... args) noexcept
    {
        shutdown();
        _active.store(true);
        _worker = std::thread{
                [&](Fn_&& fn, Args_&&... inner_args) {
                    while (_active)
                    {
                        fn(std::forward<Args_>(inner_args)...);
                    }
                },
                std::forward<Fn_>(loop_fn),
                std::forward<Args_>(args)...};
    }

    void shutdown() noexcept
    {
        _active.store(false);
        _worker.joinable() && (_worker.join(), 0);
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