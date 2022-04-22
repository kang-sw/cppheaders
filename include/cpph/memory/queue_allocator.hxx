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

/**
 * allocates byte array sequentially.
 * \code
 *   using queue_allocator = basic_queue_allocator<std::allocator<char>>;
 *   queue_allocator alloc {
 *      128,  // number of maxtimum entities
 *      8192  // maximum buffer allocations.
 *   };
 *
 *   // alloc.resize(); queue allocator doesn't provide way to resize buffer in runtime
 *   //                 if size adjustment is required, a new allocator must be assigned.
 *   alloc = basic_queue_allocator<std::allocator>{256, 65536};
 *
 *   // template<typename Ty_> Ty_& ..::push(Ty_&&);
 *   alloc.push(3);
 *
 *   // template<typename Ty_, typename... Args_>
 *   // Ty_& ..::emplace(Args_...);
 *   alloc.emplace<std::string>("hell, world!");
 *
 *   // can visit arbitrary node. [0] is same as front() (element which will be popped next)
 *   alloc.at<int>(0); // -> int&
 *   alloc.at<std::string>(1); // -> std::string&
 *
 *   alloc.pop();
 *
 *   // alloc.peek<int>(); // runtime-error; type information doesn't match
 *   alloc.peek<std::string>().clear();
 *
 *   // can perform peek and pop at the same time, before destroy.
 *   // in here, function parameter and actual type must match exactly!
 *   alloc.pop([](std::string&& s) { std::cout << s; });
 *
 *   // or simply pop.
 *   alloc.pop();
 *
 *
 *
 *   // allocate array
 *   // in this case,
 *   auto k = alloc.push_array<int>(1024);
 *   // k := array_view<int>; k.size() == 1024;
 *
 *   // ~basic_queue_allocator() -> will release all allocations automatically.
 * \endcode
 */
#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "../array_view.hxx"

//
namespace cpph {
namespace detail {
struct queue_buffer_block {
    uint32_t defferred : 1;
    uint32_t _padding  : 31;

    // there's no field to indicate next pos.
    // ((_blk_type*)_mem + size) will point next memory block header.
    // if offset + size exceeds capacity, it implicitly means its next node is _refer(0).
    uint32_t size = 0;

    bool occupied() const noexcept { return size != 0; }
};

// To avoid <memory> header dependency.
template <typename Ty_>
struct queue_buffer_default_allocator {
    auto allocate(size_t n) -> Ty_*
    {
        return (Ty_*)malloc(n * sizeof(Ty_));
    }

    void deallocate(Ty_* p)
    {
        free(p);
    }
};
}  // namespace detail

struct queue_out_of_memory : std::bad_alloc {};

namespace detail {
class queue_buffer_impl
{
   public:
    enum {
        block_size = 8
    };

   private:
    using memblk = detail::queue_buffer_block;
    static_assert(sizeof(memblk) == block_size);

   public:
    queue_buffer_impl(size_t capacity, memblk* buffer)
            : _capacity{to_block_size(capacity)},
              _mem{buffer}
    {
        memset(buffer, 0, capacity);
    }

    queue_buffer_impl() = default;

    ~queue_buffer_impl() noexcept
    {
        assert(not _mem);
        assert(empty());
    }

    void* allocate(size_t n)
    {
        return _alloc(n);
    }

    void deallcoate(void* p) noexcept
    {
        _dealloc(p);
    }

    void* front() noexcept
    {
        return (void*)(_tail + 1);
    }

    void* next(void* p) noexcept
    {
        return _next((memblk*)p - 1) + 1;
    }

    size_t capacity() const noexcept
    {
        return _capacity * block_size;
    }

    size_t size() const noexcept
    {
        return _num_alloc;
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }

   protected:
    memblk* release() noexcept
    {
        auto ref = _mem;
        _mem = nullptr;
        return ref;
    }

    constexpr static size_t to_block_size(size_t n) noexcept
    {
        return ((n + block_size - 1) / block_size);
    }

   private:
    void* _alloc(size_t n)
    {
        if (n == 0)
            throw std::invalid_argument{"bad allocation size"};

        auto num_block = to_block_size(n);

        if (not _head) {
            if (num_block + 1 > _capacity)
                throw queue_out_of_memory{};

            _head = _tail = _refer(0);
            _head->size = num_block;
        } else {
            // 1.   if there's not enough space to allocate new memory block,
            //       fill tail memory to end, and insert new item at first.
            // 2.   otherwise, simply allocate new item at candidate place.

            auto candidate = _head + _head->size + 1;

            if (_head >= _tail) {
                if (candidate + num_block >= _border()) {
                    _head->size = _border() - _head - 1;
                    candidate = _refer(0);

                    if (candidate + 1 + num_block >= _tail)
                        throw queue_out_of_memory{};
                }

                if (candidate->occupied())
                    throw queue_out_of_memory{};
            } else {
                if (candidate + num_block >= _tail)
                    throw queue_out_of_memory{};

                if (candidate->occupied())
                    throw queue_out_of_memory{};
            }

            candidate->size = num_block;
            _head = candidate;
        }

        ++_num_alloc;
        return _head + 1;
    }

    void _dealloc(void* p) noexcept
    {
        assert(p && not empty());

        if (auto* _node = (memblk*)p - 1; _node != _tail) {
            _node->defferred = true;
        } else {
            do  // perform all deferred deallocations
            {
                auto next = _next(_tail);
                *_tail = {};
                _tail = next;

                --_num_alloc;
            } while (_tail && _tail->occupied() && _tail->defferred);

            if (_tail == nullptr)
                _head = nullptr;
        }
    }

    memblk* _refer(uint32_t offset) noexcept
    {
        return _mem + offset;
    }

    memblk* _next(memblk* node) noexcept
    {
        if (node == _head)
            return nullptr;

        auto next = node + node->size + 1;

        if (next == _border())
            next = _refer(0);

        return next;
    }

    memblk* _border() const noexcept
    {
        return _mem + _capacity;
    }

   private:
    size_t _capacity = 0;
    size_t _num_alloc = 0;
    memblk* _mem = nullptr;
    memblk* _tail = nullptr;  // defined as forward-list
    memblk* _head = nullptr;
};

class basic_queue_allocator_impl
{
   public:
    explicit basic_queue_allocator_impl(detail::queue_buffer_impl* impl) noexcept
            : _impl(impl) {}

   private:
    struct alignas(8) node_type {
        void (*node_dtor)(void*, size_t n);
        size_t n = 0;
    };

    static_assert(sizeof(node_type) == 16);

    template <typename Ty_>
    class alloc_ptr
    {
       public:
        Ty_* operator->() const noexcept { return _elem; }
        Ty_& operator*() const noexcept { return *_elem; }

        alloc_ptr() noexcept = default;

        alloc_ptr(alloc_ptr&& r) noexcept : _elem(r._elem), _alloc(r._alloc)
        {
            r._elem = {}, r._alloc = {};
        }

        alloc_ptr& operator=(alloc_ptr&& r) noexcept
        {
            std::swap(_elem, r._elem);
            std::swap(_alloc, r._alloc);
            return *this;
        }

        ~alloc_ptr() noexcept
        {
            if (_alloc) {
                _alloc->destruct(_elem);
                _elem = nullptr;
                _alloc = nullptr;
            }
        }

       private:
        friend class basic_queue_allocator_impl;
        Ty_* _elem = nullptr;
        basic_queue_allocator_impl* _alloc = nullptr;
    };

    template <typename Ty_>
    class alloc_ptr<Ty_[]>
    {
       public:
        array_view<Ty_> const* operator->() const noexcept { return &_elems; }
        array_view<Ty_> const& operator*() const noexcept { return _elems; }

        alloc_ptr() noexcept = default;

        alloc_ptr(alloc_ptr&& r) noexcept : _elems(r._elems), _alloc(r._alloc)
        {
            r._elems = {}, r._alloc = {};
        }

        Ty_& operator[](size_t i) const noexcept
        {
            return _elems[i];
        }

        Ty_& at(size_t i) const
        {
            return _elems.at(i);
        }

        alloc_ptr& operator=(alloc_ptr&& r) noexcept
        {
            std::swap(_elems, r._elems);
            std::swap(_alloc, r._alloc);
            return *this;
        }

        ~alloc_ptr() noexcept
        {
            if (_alloc) {
                _alloc->destruct(_elems.data());
                _elems = {};
                _alloc = nullptr;
            }
        }

       private:
        friend class basic_queue_allocator_impl;
        array_view<Ty_> _elems = {};
        basic_queue_allocator_impl* _alloc = nullptr;
    };

   private:
    template <typename... Args_>
    size_t _get_size(size_t n, Args_&&... args)
    {
        return n;
    }

   public:
    basic_queue_allocator_impl(basic_queue_allocator_impl&&) noexcept = default;
    basic_queue_allocator_impl& operator=(basic_queue_allocator_impl&&) noexcept = default;

    template <typename Ty_,
              typename... Args_>
    Ty_* construct(Args_&&... args)
    {
        constexpr size_t alloc_size = sizeof(Ty_) + sizeof(node_type);
        auto node = (node_type*)_impl->allocate(alloc_size);
        auto memory = (Ty_*)(node + 1);

        if constexpr (std::is_trivial_v<Ty_>) {
            node->n = 0;
            node->node_dtor = [](void* p, size_t n) {};
        } else {
            node->n = 0;
            node->node_dtor = [](void* p, size_t n) {
                (*(Ty_*)p).~Ty_();
            };

            new (memory) Ty_{std::forward<Args_>(args)...};
        }

        return memory;
    }

    template <typename Ty_>
    array_view<Ty_> construct_array(size_t n)
    {
        size_t const alloc_size = sizeof(Ty_) * n + sizeof(node_type);
        auto node = (node_type*)_impl->allocate(alloc_size);
        Ty_* memory = (Ty_*)(node + 1);

        if constexpr (std::is_trivial_v<Ty_>) {
            node->n = n;
            node->node_dtor = [](void* p, size_t n) {};
        } else {
            node->n = n;
            node->node_dtor = [](void* p, size_t n) {
                while (n--)
                    ((Ty_*)p)[n].~Ty_();
            };

            for (size_t i = 0; i < n; ++i) {
                new (memory + i) Ty_{};
            }
        }

        return array_view{memory, n};
    }

    template <typename Ty_, typename... Args_>
    auto checkout(Args_&&... args)
    {
        if constexpr (std::is_array_v<Ty_>) {
            using value_type = std::remove_reference_t<decltype(std::declval<Ty_>()[0])>;
            alloc_ptr<value_type[]> ptr;
            ptr._alloc = this;
            ptr._elems = construct_array<value_type>(
                    std::get<0>(
                            std::forward_as_tuple(std::forward<Args_>(args)...)));

            return ptr;
        } else {
            alloc_ptr<Ty_> ptr;
            ptr._alloc = this;
            ptr._elem = construct<Ty_>(std::forward<Args_>(args)...);

            return ptr;
        }
    }

    void destruct(void* ptr) noexcept
    {
        auto node = _node_of(ptr);
        node->node_dtor(ptr, node->n);
        _impl->deallcoate(node);
    }

    size_t arraylen(void* ptr) const noexcept
    {
        auto node = _node_of(ptr);
        return node->n;
    }

    void* next(void* ptr) const noexcept
    {
        return (node_type*)_impl->next(_node_of(ptr)) + 1;
    }

    void* front() const noexcept
    {
        return (node_type*)_impl->front() + 1;
    }

    size_t size() const noexcept
    {
        return _impl->size();
    }

    bool empty() const noexcept
    {
        return _impl->empty();
    }

    ~basic_queue_allocator_impl() noexcept
    {
        while (not _impl->empty()) {
            destruct((node_type*)_impl->front() + 1);
            _impl->deallcoate(_impl->front());
        }
    }

   private:
    static node_type* _node_of(void* ptr) noexcept
    {
        return (node_type*)ptr - 1;
    }

   private:
    detail::queue_buffer_impl* _impl;
};

}  // namespace detail

template <typename Alloc_>
class basic_queue_buffer : public detail::queue_buffer_impl
{
   public:
    using allocator_type = Alloc_;

   public:
    basic_queue_buffer(size_t capacity, Alloc_ allocator) noexcept
            : detail::queue_buffer_impl(
                    capacity,
                    allocator.allocate(to_block_size(capacity))),
              _alloc{std::move(allocator)}
    {
    }

    explicit basic_queue_buffer(size_t capacity) noexcept
            : basic_queue_buffer(capacity, Alloc_{}) {}

    ~basic_queue_buffer() noexcept
    {
        if (auto membuf = release())
            (_alloc.deallocate(membuf), 0);
    }

    basic_queue_buffer(basic_queue_buffer&& other) noexcept
    {
        _assign(other);
    }

    basic_queue_buffer& operator=(basic_queue_buffer&& other) noexcept
    {
        ~basic_queue_buffer();
        _assign(std::move(other));
        return *this;
    }

   private:
    void _assign(basic_queue_buffer&& other) noexcept
    {
        std::swap<detail::queue_buffer_impl>(*this, other);
        std::swap(_alloc, other._alloc);
    }

   private:
    allocator_type _alloc = {};
};

template <typename Alloc_>
class basic_queue_allocator : public detail::basic_queue_allocator_impl
{
   public:
    explicit basic_queue_allocator(size_t capacity, Alloc_ allocator = {}) noexcept
            : basic_queue_allocator_impl(&_alloc),
              _alloc(capacity, allocator)
    {
    }

    basic_queue_allocator(basic_queue_allocator&&) noexcept = default;
    basic_queue_allocator& operator=(basic_queue_allocator&&) noexcept = default;

   private:
    basic_queue_buffer<Alloc_> _alloc;
};

using queue_buffer = basic_queue_buffer<
        detail::queue_buffer_default_allocator<
                detail::queue_buffer_block>>;

using queue_allocator = basic_queue_allocator<
        detail::queue_buffer_default_allocator<
                detail::queue_buffer_block>>;

}  // namespace cpph