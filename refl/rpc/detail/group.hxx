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
#include <set>

#include "../../../memory/pool.hxx"
#include "../../__namespace__"
#include "session.hxx"

namespace CPPHEADERS_NS_::rpc {
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
    container_type   _sessions;
    std::mutex       _mtx;
    pool<array_type> _tmp_pool;

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
                if ((**iter).expired())
                    iter = _sessions.erase(iter);
                else if (fn(&**iter))
                    parr->emplace_back(*iter++);
            }
        }

        for (auto& session : *parr) {
            session->notify(method, params...);
        }

        return parr->size();
    }

    template <typename... Params>
    size_t notify(string_view method, Params const&... params)
    {
        return notify_filter(
                method, [](auto&&) { return true; }, params...);
    }

    bool add_session(session_ptr ptr)
    {
        assert(ptr);
        if (ptr->expired()) { return false; }

        lock_guard _{_mtx};
        auto [iter, is_new] = _sessions.insert(std::move(ptr));

        return is_new;
    }

    bool remove_session(session_ptr ptr)
    {
        lock_guard _{_mtx};
        return _sessions.erase(ptr) != 0;
    }
};
}  // namespace CPPHEADERS_NS_::rpc
