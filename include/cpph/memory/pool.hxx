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
#include <memory>
#include <stdexcept>
#include <vector>

#include "cpph/spinlock.hxx"
#include "cpph/utility/functional.hxx"

//

namespace cpph {
using std::list;
using std::vector;

namespace _tr {
namespace _detail {
struct resource_node_header {
    resource_node_header* prev;
    resource_node_header* next;
};

template <typename Value>
struct resource_node {
    void* owner = nullptr;
    void (*fn_checkin)(void*, _detail::resource_node_header*) noexcept = nullptr;
    resource_node_header header;
    Value data;
};
}  // namespace _detail

template <typename Value>
class pool_ptr
{
    using node_type = _detail::resource_node<Value>;

   private:
    node_type* _node = nullptr;
    Value* _value = nullptr;

   public:
    pool_ptr() noexcept = default;

    pool_ptr(pool_ptr&& other) noexcept
            : _node(other._node),
              _value(other._value)
    {
        memset(&other, 0, sizeof other);
    }

    template <typename OtherValue,
              class = enable_if_t<not is_same_v<Value, OtherValue> && is_base_of_v<Value, OtherValue>>>
    pool_ptr(pool_ptr<OtherValue>&& other)
            : _node(other._node),
              _value(other._value)
    {
        memset(&other, 0, sizeof other);
    }

    pool_ptr& operator=(pool_ptr&& other) noexcept
    {
        _release();
        _node = other._node;
        _value = other._value;
        memset(&other, 0, sizeof other);
    }

    template <typename OtherValue,
              class = enable_if_t<not is_same_v<Value, OtherValue> && is_base_of_v<Value, OtherValue>>>
    pool_ptr& operator=(pool_ptr<OtherValue>&& other)
    {
        _release();
        _node = other._node;
        _value = other._value;
        memset(&other, 0, sizeof other);
    }

    ~pool_ptr() noexcept
    {
        _release();
    }

    void reset() noexcept
    {
        _release();
        _node = nullptr;
        _value = nullptr;
    }

    shared_ptr<Value> share() && noexcept
    {
        auto pointer = _value;
        return shared_ptr<Value>{pointer, [_ = move(*this)](auto) {}};
    }

   public:
    bool operator==(nullptr_t) const noexcept { return _node == nullptr; }
    bool operator!=(nullptr_t) const noexcept { return _node != nullptr; }
    friend bool operator==(nullptr_t n, pool_ptr p) noexcept { return n == p; }
    friend bool operator!=(nullptr_t n, pool_ptr p) noexcept { return n != p; }
    explicit operator bool() const noexcept { return *this != nullptr; }

    auto& operator*() noexcept { return *_value; }
    auto& operator*() const noexcept { return *_value; }
    auto* operator->() noexcept { return _value; }
    auto* operator->() const noexcept { return _value; }
    auto* get() const noexcept { return _value; }

   private:
    void _release() noexcept
    {
        if (_node) { _node->fn_checkin(_node->owner); }
    }

   public:
   public:
};

template <typename Value, typename Mutex, typename CleanUpFunc>
class basic_resource_pool
{
    using node_type = _tr::_detail::resource_node<Value>;

   private:
    node_type* _in_use;
    node_type* _in_free;
    function<node_type*()> _fn_construct;

   public:
};
}  // namespace _tr

template <typename Ty_, typename Mutex_ = null_mutex>
class basic_resource_pool
{
   public:
    using buffer_type = list<Ty_>;
    using buffer_iterator = typename buffer_type::iterator;

   public:
    struct handle_type {
        class deleter
        {
            handle_type _handle;

           public:
            using pointer = Ty_*;

            deleter(handle_type&& h) noexcept : _handle(move(h)) {}
            void operator()(pointer) { _handle.checkin(); }
        };

       public:
        handle_type() noexcept = default;

        handle_type(handle_type&& r) noexcept
        {
            _assign(move(r));
        }

        handle_type& operator=(handle_type&& r) noexcept
        {
            this->~handle_type();
            _assign(move(r));
            return *this;
        }

        ~handle_type() noexcept
        {
            valid() && (checkin(), 0);
        }

        void checkin()
        {
            _owner->checkin(move(*this));
        }

        bool valid() const noexcept
        {
            return _owner;
        }

        Ty_* operator->() const noexcept
        {
            return &*_ref;
        }

        Ty_& operator*() const noexcept
        {
            return *_ref;
        }

        auto get() const noexcept
        {
            return &*_ref;
        }

        auto share() && noexcept
        {
            // Backup pointer before move 'this' reference
            auto pointer = &**this;

            return shared_ptr<Ty_>{
                    pointer, deleter{move(*this)}};
        }

        auto unique() && noexcept
        {
            auto pointer = &**this;

            return unique_ptr<Ty_, deleter>{
                    pointer, deleter{move(*this)}};
        }

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

        basic_resource_pool* _owner = nullptr;
        typename list<Ty_>::iterator _ref{};
    };

   public:
    handle_type checkout()
    {
        lock_guard _{_mut};
        handle_type r;
        r._owner = this;

        if (_free.empty())
            _fn_construct_back();

        r._ref = _free.begin();
        _pool.splice(_pool.end(), _free, r._ref);
        return r;
    }

    void checkin(handle_type h)
    {
        if (not h.valid())
            throw std::invalid_argument{"invalid handle"};

        if (h._owner != this)
            throw std::invalid_argument{"invalid owner reference"};

        lock_guard _{_mut};
        _free.splice(_free.begin(), _pool, h._ref);

        h._owner = {};
        h._ref = {};
    }

    void shrink()
    {
        lock_guard _{_mut};
        _free.clear();
    }

    //! Check if not any resource is currently checked out.
    bool empty()
    {
        lock_guard _{_mut};
        return _pool.empty();
    }

    basic_resource_pool()
    {
        _fn_construct_back =
                [this] {
                    _free.emplace_back();
                };
    }

    ~basic_resource_pool()
    {
        // TODO: way to detect deadlock? (e.g. from destruction order in same thread ...)

        // Yield for long time, since if following empty() call returns true, it normally
        //  indicates async process is running from another thread,
        while (not empty()) { _detail::thread_yield(); }
    }

    template <typename... Args_>
    explicit basic_resource_pool(Args_&&... args)
    {
        _fn_construct_back =
                [this, tup_args = forward_as_tuple(args...)] {
                    auto fn_apply =
                            [this](auto&&... args) {
                                _free.emplace_back(forward<decltype(args)>(args)...);
                            };
                    apply(fn_apply, tup_args);
                };
    }

   private:
    Mutex_ _mut;

    function<void()> _fn_construct_back;
    list<Ty_> _pool;
    list<Ty_> _free;
};

template <typename Ty_>
using pool = basic_resource_pool<Ty_, spinlock>;

template <typename Ty_>
using pool_ptr = typename pool<Ty_>::handle_type;

}  // namespace cpph
