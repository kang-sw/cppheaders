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
#include <memory>

#include "cpph/thread/spinlock.hxx"
#include "cpph/utility/template_utils.hxx"

//

namespace cpph {
namespace _tr {
namespace _detail {
struct pool_node {
    weak_ptr<class pool_base> owner = {};
    pool_node* next = {};
    char data[];

   public:
    template <typename T>
    static void dispose(pool_node* n)
    {
        reinterpret_cast<T*>(n->data)->~T();
        erase_memory(n);
    }

    static void erase_memory(pool_node* n)
    {
        assert(n->next == nullptr);

        n->destruct_header();
        delete[] reinterpret_cast<char*>(n);
    }

    void destruct_header()
    {
        owner.~weak_ptr<pool_base>();
    }

    template <typename T>
    T* get() noexcept
    {
        return reinterpret_cast<T*>(data);
    }
};

class if_pool_mutex
{
   public:
    virtual ~if_pool_mutex() = default;
    virtual void lock() noexcept = 0;
    virtual void unlock() noexcept = 0;
};

template <typename Mutex>
class pool_mutex : public if_pool_mutex
{
    Mutex _mtx;

   public:
    void lock() noexcept override
    {
        _mtx.lock();
    }

    void unlock() noexcept override
    {
        _mtx.unlock();
    }
};

class pool_base : public std::enable_shared_from_this<pool_base>
{
    pool_node* _idle_nodes = nullptr;

    ptr<if_pool_mutex> _mtx;
    void (*_dtor)(void*) noexcept;

   public:
    pool_base(ptr<if_pool_mutex> mtx, decltype(_dtor) dtor) noexcept : _mtx(move(mtx)), _dtor(dtor)
    {
        assert(_mtx);
        assert(_dtor);
    }

    ~pool_base() noexcept
    {
        clear_all_idle();
    }

   public:
    pool_node* try_checkout() noexcept
    {
        assert(_mtx != nullptr);
        lock_guard _{*_mtx};

        if (not _idle_nodes)
            return nullptr;

        auto node = exchange(_idle_nodes, _idle_nodes->next);
        node->next = nullptr;

        return node;
    }

    template <typename T, typename... Args>
    pool_node* construct(Args&&... args)
    {
        auto node_mem = new char[sizeof(pool_node) + sizeof(T)];
        auto node = reinterpret_cast<pool_node*>(node_mem);

        new (node) pool_node();

        try {
            new (node->data) T{std::forward<Args>(args)...};
        } catch (...) {
            node->destruct_header();
            delete[] node_mem;
            throw;
        }

        node->owner = weak_from_this();
        return node;
    }

    void checkin(pool_node* n) noexcept
    {
        assert(n->next == nullptr);
        assert(_mtx != nullptr);
        lock_guard _{*_mtx};

        if (_idle_nodes) {
            n->next = exchange(_idle_nodes, n);
        } else {
            _idle_nodes = n;
        }
    }

    void clear_all_idle() noexcept
    {
        pool_node* node = nullptr;
        {
            lock_guard _{*_mtx};
            node = exchange(_idle_nodes, nullptr);
        }

        for (pool_node* next; node; node = next) {
            next = exchange(node->next, nullptr);

            _dtor(node->data);
            pool_node::erase_memory(node);
        }
    }
};

}  // namespace _detail

template <typename T>
class pool_ptr
{
   public:
    using value_type = T;

   private:
    _detail::pool_node* _node = nullptr;

   public:
    pool_ptr() noexcept = default;
    pool_ptr(pool_ptr&& other) noexcept : _node(exchange(other._node, nullptr)) {}
    pool_ptr& operator=(pool_ptr&& other) noexcept { return swap(_node, other._node), *this; }

    explicit pool_ptr(_detail::pool_node* h) : _node(h) {}

    void checkin() noexcept
    {
        if (auto node = exchange(_node, nullptr)) {
            if (auto owner = node->owner.lock()) {
                owner->checkin(node);
            } else {
                _detail::pool_node::dispose<T>(node);
            }
        }
    }

    T* get() const noexcept
    {
        if (_node) {
            return _node->template get<T>();
        } else {
            return nullptr;
        }
    }

    bool valid() const noexcept { return _node; }
    explicit operator bool() const noexcept { return valid(); }
    T* operator->() const noexcept { return get(); }
    T& operator*() const noexcept { return *get(); }
    auto to_const() && noexcept { return pool_ptr<T const>(move(*this)); }
    operator pool_ptr<T const>() && noexcept { return to_const(); }

    shared_ptr<T> share() &&
    {
        if (not _node)
            return nullptr;

        auto data = get();
        return shared_ptr<T>{data, [disposer = move(*this)](auto) {}};
    }

    auto unique() && noexcept;

    void detach()
    {
        _node->owner.reset();
    }

   public:
    _detail::pool_node* _internal_handle() const noexcept { return _node; }

   public:
    ~pool_ptr() noexcept(std::is_nothrow_destructible_v<T>)
    {
        checkin();
    }
};

template <typename T>
struct _pool_ptr_disposer {
    using pointer = T*;
    pool_ptr<T> _node;
    void operator()(pointer) const noexcept {}
};

template <typename T>
using unique_pool_ptr = unique_ptr<T, _pool_ptr_disposer<remove_const_t<T>>>;

template <typename T>
auto pool_ptr<T>::unique() && noexcept
{
    auto data = this->get();
    return unique_pool_ptr<T>{data, {move(*this)}};
}

template <typename T, class Mutex = spinlock, typename... Params>
class pool : public tuple<Params...>
{
    shared_ptr<_detail::pool_base> _pool
            = make_shared<_detail::pool_base>(
                    make_unique<_detail::pool_mutex<Mutex>>(), &pool::_dtor);

   public:
    pool() noexcept = default;
    pool(pool&&) noexcept = default;
    pool& operator=(pool&&) noexcept = default;

    using tuple<Params...>::tuple;

   private:
    static void _dtor(void* p) noexcept
    {
        reinterpret_cast<T*>(p)->~T();
    }

   public:
    using handle_type = pool_ptr<T>;

   public:
    handle_type checkout()
    {
        if (auto node = _pool->try_checkout()) {
            return handle_type{node};
        } else {
            return handle_type{std::apply(
                    [this](auto&&... params) {
                        return _pool->template construct<T>(params...);
                    },
                    static_cast<tuple<Params...>&>(*this))};
        }
    }

    void checkin(handle_type h) noexcept
    {
        h.checkin();
    }

    void shrink() noexcept
    {
        _pool->clear_all_idle();
    }

    handle_type take_ownership(handle_type other) noexcept
    {
        other._internal_handle()->owner = _pool->weak_from_this();
        return move(other);
    }
};
}  // namespace _tr

using _tr::pool;
using _tr::pool_ptr;
using _tr::unique_pool_ptr;
}  // namespace cpph
