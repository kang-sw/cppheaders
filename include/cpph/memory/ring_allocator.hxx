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
#include <cstring>
#include <memory>
#include <stdexcept>

#include "cpph/utility/generic.hxx"

//
namespace cpph {
namespace _detail {
template <typename T>
class ring_fallback_allocator : private T
{
    enum { elem_size = sizeof(typename T::value_type) };

   protected:
    void* _allocate_fallback(size_t nbytes)
    {
        return this->allocate((nbytes + elem_size - 1) / elem_size);
    }

    void _deallocate_fallback(void* p, size_t nbytes)
    {
        this->deallocate((typename T::pointer)p, nbytes / elem_size);
    }

   public:
    T& allocator() noexcept
    {
        return *this;
    }
};

template <>
class ring_fallback_allocator<void>
{
};

struct ring_node_t {
    uint64_t pending_kill       : 1;
    uint64_t fallback_allocated : 1;
    uint64_t _reserved          : 2;

    // in block count
    uint64_t extent : 60;
    char buffer[0];

    ring_node_t* next_node() const noexcept { return (ring_node_t*)buffer + extent; }
};

}  // namespace _detail

/**
 * Ring-shaped allocator
 */
template <typename FallbackAlloc = void>
class basic_ring_allocator : private _detail::ring_fallback_allocator<FallbackAlloc>
{
    using node_t = _detail::ring_node_t;
    static_assert(sizeof(node_t) == 8);

   public:
    static constexpr auto has_fallback_allocator = not is_same_v<FallbackAlloc, void>;
    static constexpr auto node_size = sizeof(node_t);

   private:
    node_t* _memory = {};
    size_t _capacity = {};
    node_t* _head = {};
    node_t* _tail = {};

    void (*_dealloc)(void*, void*) = {};
    void* _user = {};

   public:
    explicit basic_ring_allocator(void* buffer, size_t size, void (*dealloc)(void*, void*), void* user) noexcept
            : _memory((node_t*)buffer),
              _capacity(_node_size_floor(size)),
              _dealloc(dealloc),
              _user(user)
    {
        assert(_memory != nullptr);
        assert(_capacity > 0);
        assert(_dealloc != nullptr);

        memset(buffer, 0, size);
    }

    explicit basic_ring_allocator(void* buffer, size_t size, void (*dealloc)(void*)) noexcept
            : basic_ring_allocator(
                    buffer, size,
                    [](void* p, void* vfn) { ((void (*)(void*))vfn)(p); },
                    decltype(_user)(dealloc))
    {
    }

    explicit basic_ring_allocator(size_t size) noexcept
            : basic_ring_allocator(malloc(_node_size_ceil(size) * node_size), _node_size_ceil(size) * node_size, [](void* p) { free(p); })
    {
    }

    basic_ring_allocator() noexcept
            : _dealloc([](auto, auto) {})
    {
    }

    ~basic_ring_allocator() noexcept
    {
        _dealloc(_memory, _user);
    }

   public:
    basic_ring_allocator(basic_ring_allocator&& other) noexcept { *this = move(other); }
    basic_ring_allocator& operator=(basic_ring_allocator&& other) noexcept
    {
        char tmp[sizeof *this];

        // Swap all entities
        memcpy(tmp, this, sizeof *this);
        memcpy(this, &other, sizeof *this);
        memcpy(&other, tmp, sizeof *this);

        return *this;
    }

   public:
    size_t capacity() const noexcept
    {
        return _capacity * node_size;
    }

    void* allocate(size_t n)
    {
        if (auto ptr = allocate_nt(n)) {
            return ptr;
        } else {
            throw std::bad_alloc{};
        }
    }

    void* allocate_nt(size_t n) noexcept
    {
        auto vp = _allocate_ring(n);

        if constexpr (has_fallback_allocator) {
            n = _node_size_ceil(n);
            if (auto node = (node_t*)this->_allocate_fallback((n + 1) * node_size)) {
                vp = node + 1;
                node->fallback_allocated = true;
                node->pending_kill = false;
                node->extent = n;
            }
        }

        return vp;
    }

    void* allocate(size_t n, std::nothrow_t) noexcept
    {
        return allocate_nt(n);
    }

    void deallocate(void* vp) noexcept
    {
        if constexpr (has_fallback_allocator) {
            if (is_ring_allocated(vp)) {
                _deallocate_ring(vp);
            } else {
                auto node = _retr_node(vp);
                this->_deallocate_fallback(node, node->extent * node_size);
            }
        } else {
            _deallocate_ring(vp);
        }
    }

    bool empty() const noexcept
    {
        return _head == nullptr;
    }

    void* front()
    {
        return _tail->buffer;
    }

    bool is_ring_allocated(void* p) const noexcept
    {
        if constexpr (has_fallback_allocator) {
            auto node = _retr_node(p);
            if (not node->fallback_allocated) {
                assert(_memory <= node && node < _memory + _capacity && "Node must be placed in memory range!");
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    static size_t extent(void* memory) noexcept
    {
        return _retr_node(memory)->extent * node_size;
    }

   private:
    void* _allocate_ring(size_t n) noexcept
    {
        assert(_memory);

        auto const nblk = _node_size_ceil(n);
        void* data_alloc = nullptr;

        if (_head == nullptr) {
            if (nblk >= _capacity) { return nullptr; }  // Out of memory
            _tail = _head = _memory;

            _head->pending_kill = false;
            _head->fallback_allocated = false;
            _head->extent = nblk;
            data_alloc = _head->buffer;

            _head = _head->next_node();
        } else if (_head > _tail) {
            // 1. First, try to allocate memory from remaining space to end
            auto end = _memory + _capacity;
            auto space = end - (node_t*)_head->buffer;

            if (nblk <= space) {
                // Can allocate memory here.
                _head->pending_kill = false;
                _head->fallback_allocated = false;
                _head->extent = nblk;
                data_alloc = _head->buffer;

                _head = _head->next_node();
            } else if (_tail != _memory) {
                // 2. Fill dummy element in head, jump to front.
                _head->pending_kill = true;
                _head->fallback_allocated = false;
                _head->extent = space;

                // Retry allocation from the first point
                _head = _memory;
                return _allocate_ring(n);
            } else {
                // If head cannot even jump to front, buffer is full.
                return nullptr;
            }
        } else if (_head < _tail) {
            auto space = _tail - _head - 1;

            if (nblk > space) {
                // Simply, there's no more space.
                return nullptr;
            }

            _head->pending_kill = false;
            _head->fallback_allocated = false;
            _head->extent = nblk;
            data_alloc = _head->buffer;

            _head = _head + 1 + nblk;
        } else /*if (_head == _tail)*/ {
            // Buffer is fullfilled.
            return nullptr;
        }

        if (_head == _memory + _capacity) {
            _head = _memory;
        }

        assert(data_alloc);
        return data_alloc;
    }

    void _deallocate_ring(void* p)
    {
        auto node = _retr_node(p);
        assert(p && not empty());
        assert(_memory <= node && node < _memory + _capacity);

        // Mark this node to be killed
        node->pending_kill = true;

        // GC all disposing nodes
        while (_tail && _tail->pending_kill) {
            assert(not _tail->fallback_allocated);
            _tail = _tail->next_node();  // actual deallocation

            if (_tail == _memory + _capacity) {
                // If tail reaches end of the buffer, go to front again.
                _tail = _memory;
            }

            if (_tail == _head) {
                // If tail reaches head, it means all nodes has been disposed.
                _tail = _head = nullptr;
            }
        }
    }

   private:
    static constexpr size_t _node_size_ceil(size_t s) { return ((s + (node_size - 1)) & ~(node_size - 1)) / node_size; }
    static constexpr size_t _node_size_floor(size_t s) { return s / node_size; }

    static node_t* _retr_node(void* data) noexcept { return (node_t*)data - 1; }
};

using ring_allocator = basic_ring_allocator<>;
using ring_allocator_with_fb = basic_ring_allocator<std::allocator<char>>;
}  // namespace cpph