/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2021. Seungwoo Kang
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
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

#include "../macros.hxx"

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_ {
template <typename Ty_>
class notify_queue
{
   public:
    using value_type = Ty_;

   private:
    using _lc_t = std::unique_lock<std::mutex>;

   public:
    void set_capacity(size_t max)
    {
        _lc_t lc{_mtx};
        _cap = max;
    }

    template <typename... Args_>
    void emplace(Args_&&... args)
    {
        _lc_t lc{_mtx};

        while (_queue.size() + 1 >= _cap)
            _queue.pop_front();

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
    volatile size_t _cap = ~size_t{};
    std::condition_variable _cvar;
};
}  // namespace CPPHEADERS_NS_