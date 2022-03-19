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
#include <list>
#include <stdexcept>
#include <thread>
#include <vector>

#include "../functional.hxx"
#include "../spinlock.hxx"

//
#include "../__namespace__"

namespace CPPHEADERS_NS_ {
template <typename Ty_, typename Mutex_ = null_mutex>
class basic_resource_pool
{
   public:
    using buffer_type = std::list<Ty_>;
    using buffer_iterator = typename buffer_type::iterator;

   public:
    struct handle_type
    {
       public:
        handle_type() noexcept = default;

        handle_type(handle_type&& r) noexcept
        {
            _assign(std::move(r));
        }

        handle_type& operator=(handle_type&& r) noexcept
        {
            this->~handle_type();
            _assign(std::move(r));
            return *this;
        }

        ~handle_type() noexcept
        {
            valid() && (checkin(), 0);
        }

        void checkin()
        {
            _owner->checkin(std::move(*this));
        }

        bool valid() const noexcept
        {
            return _owner;
        }

        Ty_* operator->() noexcept
        {
            return &*_ref;
        }

        Ty_ const* operator->() const noexcept
        {
            return &*_ref;
        }

        Ty_& operator*() noexcept
        {
            return *_ref;
        }

        Ty_ const& operator*() const noexcept
        {
            return *_ref;
        }

        std::shared_ptr<Ty_> share() && noexcept;

       private:
        void _assign(handle_type&& other)
        {
            _owner = other._owner;
            _ref = other._ref;
            other._owner = nullptr;
            other._ref = {};
        }

       private:
        friend class basic_resource_pool;

        basic_resource_pool*              _owner = nullptr;
        typename std::list<Ty_>::iterator _ref{};
    };

    struct shared_handle_deleter
    {
        using pointer = Ty_*;
        handle_type handle;

        void        operator()(pointer)
        {
            handle.checkin();
        }
    };

   public:
    handle_type checkout()
    {
        lock_guard  _{_mut};
        handle_type r;
        r._owner = this;

        if (not _free.empty()) {
            r._ref = _free.back();
            _free.pop_back();
            return r;
        }

        _constructor();
        r._ref = _pool.begin();
        return r;
    }

    void checkin(handle_type h)
    {
        if (not h.valid())
            throw std::invalid_argument{"invalid handle"};

        if (h._owner != this)
            throw std::invalid_argument{"invalid owner reference"};

        lock_guard _{_mut};
        _free.push_back(h._ref);

        h._owner = {};
        h._ref = {};
    }

    void shrink()
    {
        lock_guard _{_mut};

        for (auto it : _free)
            _pool.erase(it);
    }

    //! Check if not any resource is currently checked out.
    bool empty()
    {
        lock_guard _{_mut};

        return _pool.size() == _free.size();
    }

    basic_resource_pool()
    {
        _constructor = [this] {
            _pool.emplace_front();
        };
    }

    ~basic_resource_pool()
    {
        // TODO: way to detect deadlock? (e.g. from destruction order in same thread ...)

        // Yield for long time, since if following empty() call returns true, it normally
        //  indicates async process is running from another thread,
        while (not empty()) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    }

    template <typename... Args_>
    explicit basic_resource_pool(Args_&&... args)
    {
        _constructor =
                [this,
                 tup_args = std::forward_as_tuple(args...)] {
                    std::apply(
                            [this](auto&&... args) {
                                _pool.emplace_front(std::forward<decltype(args)>(args)...);
                            },
                            tup_args);
                };
    }

   private:
    Mutex_                       _mut;

    perfkit::function<void()>    _constructor;
    std::list<Ty_>               _pool;
    std::vector<buffer_iterator> _free;
};

template <typename Ty_, typename Mutex_>
std::shared_ptr<Ty_> basic_resource_pool<Ty_, Mutex_>::handle_type::share() && noexcept
{
    auto ptr = &**this;

    return std::shared_ptr<Ty_>{
            ptr,
            shared_handle_deleter{std::move(*this)}};
}

template <typename Ty_>
using pool = basic_resource_pool<Ty_, spinlock>;

template <typename Ty_>
using pool_ptr = typename pool<Ty_>::handle_type;

}  // namespace CPPHEADERS_NS_
