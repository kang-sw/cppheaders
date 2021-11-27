#pragma once
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

#include "../macros.hxx"

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_
{
template <typename Ty_>
class notify_queue
{
   public:
    using value_type = Ty_;

   private:
    using _lc_t = std::unique_lock<std::mutex>;

   public:
    template <typename... Args_>
    void emplace(Args_&&... args)
    {
        _lc_t lc{_mtx};
        _queue.emplace_back(std::forward<Args_>(args)...);
        _cvar.notify_one();
    }

    void push(Ty_ const& value)
    {
        emplace(value);
    }

    void push(Ty_&& value)
    {
        emplace(std::move(value));
    }

    template <typename Duration_>
    std::optional<Ty_> try_pop(Duration_ wait)
    {
        _lc_t lc{_mtx};
        std::optional<Ty_> value;

        if (_cvar.wait_for(lc, wait, CPPH_BIND(_not_empty)))
        {
            value.emplace(std::move(_queue.front()));
            _queue.pop_front();
        }

        return value;
    }

   private:
    bool _not_empty() const noexcept
    {
        return not _queue.empty();
    }

   private:
    std::deque<Ty_> _queue;
    std::mutex _mtx;
    std::condition_variable _cvar;
};
}  // namespace CPPHEADERS_NS_