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
#include <cpph/std/set>

#include "../../../memory/pool.hxx"
#include "session.hxx"

namespace cpph::rpc {
/**
 * Container of multiple sessions.
 *
 * Naively implements list of sessions
 */
class session_group
{
    using container_type = std::set<session_ptr, std::owner_less<>>;
    using array_type = std::vector<session_ptr>;

   private:
    mutable std::mutex _mtx;
    container_type _sessions;
    pool<array_type> _tmp_pool;

    size_t _rt_off = 0, _wt_off = 0;

   public:
    template <typename... Params, typename Filter>
    size_t notify_filter(string_view method, Filter&& fn, Params const&... params)
    {
        auto parr = _tmp_pool.checkout();
        {
            lock_guard _{_mtx};
            parr->clear();
            parr->reserve(_sessions.size());

            for (auto iter = _sessions.begin(); iter != _sessions.end();) {
                if ((**iter).expired()) {
                    // Accumulate total rw bytes
                    size_t rt, wt;
                    (**iter).totals(&rt, &wt);

                    _rt_off += rt, _wt_off += wt;

                    // Erase from list
                    iter = _sessions.erase(iter);
                } else {
                    parr->emplace_back(*iter++);
                }
            }
        }

        for (auto& session : *parr)
            if (fn(session->profile()))
                session->notify(method, params...);

        return parr->size();
    }

    template <typename... Params>
    size_t notify(string_view method, Params const&... params)
    {
        return notify_filter(
                method, [](auto&&) { return true; }, params...);
    }

    void gc()
    {
        lock_guard _{_mtx};

        for (auto iter = _sessions.begin(); iter != _sessions.end();)
            if ((**iter).expired()) {
                // Accumulate total rw bytes
                size_t rt, wt;
                (**iter).totals(&rt, &wt);

                _rt_off += rt, _wt_off += wt;

                // Erase from list
                iter = _sessions.erase(iter);
            } else {
                ++iter;
            }
    }

    bool add_session(session_ptr ptr)
    {
        assert(ptr);
        if (ptr->expired()) { return false; }

        bool is_new;
        {
            lock_guard _{_mtx};
            is_new = _sessions.insert(ptr).second;
        }

        // Offset start value
        if (is_new) {
            size_t rd, wr;
            ptr->totals(&rd, &wr);
            _rt_off -= rd, _wt_off -= wr;
        }

        return is_new;
    }

    bool remove_session(weak_ptr<session> const& ptr)
    {
        lock_guard _{_mtx};
        auto iter = _sessions.find(ptr);

        if (iter == _sessions.end())
            return false;

        // Offset end value
        size_t rd, wr;
        (**iter).totals(&rd, &wr);
        _rt_off += rd, _wt_off += wr;

        _sessions.erase(iter);

        return true;
    }

    auto release()
    {
        lock_guard _{_mtx};

        // Recalculate accumulated I/O byte counts
        size_t wt, rt;
        _totals_impl(&rt, &wt);
        _rt_off = rt, _wt_off = wt;

        return std::exchange(_sessions, {});
    }

    size_t size() const noexcept
    {
        lock_guard _{_mtx};
        return _sessions.size();
    }

    void totals(size_t* nread, size_t* nwrite) const
    {
        lock_guard _{_mtx};
        _totals_impl(nread, nwrite);
    }

   private:
    void _totals_impl(size_t* nread, size_t* nwrite) const
    {
        size_t rt = 0, wt = 0;
        for (auto& sess : _sessions) {
            size_t r, w;
            sess->totals(&r, &w);
            rt += r, wt += w;
        }

        *nread = rt + _rt_off;
        *nwrite = wt + _wt_off;
    }
};
}  // namespace cpph::rpc
