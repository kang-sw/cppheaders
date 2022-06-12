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

#include "cpph/thread/spinlock.hxx"
#include "cpph/utility/functional.hxx"

//

namespace cpph {
using std::list;
using std::vector;

namespace _tr {
namespace _detail {
class pool_base;

struct pool_node {
    weak_ptr<pool_base> owner = {};
    pool_node* next = {};
    char data[];

   public:
    template <typename T>
    static void dispose(pool_node* n)
    {
        static_cast<T*>(n->data)->~T();
        dispose_header(n);
    }

    static void dispose_header(pool_node* n)
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
    T* get() const noexcept
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

class pool_base : enable_shared_from_this<pool_base>
{
    pool_node* _idle_nodes = nullptr;

    ptr<if_pool_mutex> _mtx;
    void (*_dtor)(void*);

   public:
    pool_base(ptr<if_pool_mutex> mtx, decltype(_dtor) dtor) noexcept : _mtx(move(mtx)), _dtor(dtor)
    {
        assert(_mtx);
        assert(_dtor);
    }

    ~pool_base()
    {
        clear_all_idle();
    }

   public:
    pool_node* try_checkout() noexcept
    {
        assert(_mtx != nullptr);
        CPPH_TMPVAR = lock_guard{*_mtx};

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

    void checkin(pool_node*& n) noexcept
    {
        assert(n->next == nullptr);
        assert(_mtx != nullptr);
        CPPH_TMPVAR = lock_guard{*_mtx};

        if (_idle_nodes) {
            n->next = exchange(_idle_nodes, n);
        } else {
            _idle_nodes = n;
        }

        n = nullptr;
    }

    void clear_all_idle()
    {
        pool_node* node = nullptr;
        {
            CPPH_TMPVAR = lock_guard{*_mtx};
            node = exchange(_idle_nodes, nullptr);
        }

        for (pool_node* next; node; node = next) {
            next = node->next;

            _dtor(node->data);
            pool_node::dispose_header(node);
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
    pool_ptr& operator=(pool_ptr&& other) noexcept { swap(_node, other._node); }
    explicit pool_ptr(_detail::pool_node* h) : _node(h) {}

    void checkin() noexcept
    {
        assert(_node);

        if (auto owner = _node->owner.lock()) {
            owner->checkin(_node);
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
    auto operator->() const noexcept { return get(); }
    auto& operator*() const noexcept { return *get(); }

    shared_ptr<T> share() &&
    {
        if (not _node)
            return nullptr;

        auto data = get();
        return shared_ptr<T>{data, [disposer = move(*this)](auto) {}};
    }

   public:
    ~pool_ptr() noexcept(std::is_nothrow_destructible_v<T>)
    {
        if (auto node = exchange(_node, nullptr)) {
            if (auto owner = node->owner.lock()) {
                owner->checkin(node);
            } else {
                _detail::pool_node::dispose<T>(node);
            }
        }
    }
};

template <typename T>
struct _pool_ptr_disposer {
    using pointer = T*;
    pool_ptr<T> _node;
    void operator()(pointer) const noexcept {}
};

template <typename T>
using unique_pool_ptr = unique_ptr<pool_ptr<T>, _pool_ptr_disposer<T>>;

template <typename T, class Mutex>
class basic_resource_pool
{
    shared_ptr<_detail::pool_base> _pool = make_shared<_detail::pool_base>(
            make_unique<_detail::pool_mutex<Mutex>>(), &basic_resource_pool::_dtor);

    function<_detail::pool_node*()> _construct_new_node = [this] {
        return _pool->construct<T>();
    };

   public:
    basic_resource_pool() noexcept = default;

    template <typename Param0, typename... Params>
    explicit basic_resource_pool(Param0&& p0, Params&&... params)
            : _construct_new_node([this, params = make_tuple(std::forward<Param0>(p0), std::forward<Params>(params)...)] {
                  apply(
                          [this](auto&&... params) {
                              return _pool->template construct<T>(params...);
                          },
                          params);
              })
    {
    }

   private:
    static void _dtor(void* p)
    {
        reinterpret_cast<T*>(p)->~T();
    }

   public:
    using handle_type = pool_ptr<T>;

   public:
    handle_type checkout()
    {
        if (auto node = _pool->try_checkout()) {
            return {node};
        } else {
            return {_construct_new_node()};
        }
    }

    void checkin(handle_type h)
    {
        h.checkin();
    }

    void shrink()
    {
        _pool->clear_all_idle();
    }
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
