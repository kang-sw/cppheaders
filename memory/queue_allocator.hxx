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
#include <stdexcept>
#include <utility>

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_
{
namespace detail
{
struct queue_buffer_block
{
    uint32_t defferred : 1;
    uint32_t _padding  : 31;

    // there's no field to indicate next pos.
    // ((_blk_type*)_mem + size) will point next memory block header.
    // if offset + size exceeds capacity, it implicitly means its next node is _refer(0).
    uint32_t size = 0;

    bool occupied() const noexcept { return size != 0; }
};

// To avoid <memory> header dependency.
struct queue_buffer_default_allocator
{
    auto allocate(size_t n) -> queue_buffer_block*
    {
        return (queue_buffer_block*)malloc(n * sizeof(queue_buffer_block));
    }

    void deallocate(queue_buffer_block* p)
    {
        free(p);
    }
};
}  // namespace detail

struct queue_out_of_memory : std::bad_alloc
{
};

template <class Alloc_ = detail::queue_buffer_default_allocator>
class basic_queue_buffer
{
   public:
    using allocator_type = Alloc_;
    enum
    {
        block_size = 8
    };

   private:
    using memblk = detail::queue_buffer_block;
    static_assert(sizeof(memblk) == block_size);

   public:
    basic_queue_buffer(size_t capacity, Alloc_ allocator)
            : _allocator{std::move(allocator)},
              _capacity{_size_in_blocks(capacity)},
              _mem{_allocator.allocate(_capacity)}
    {
        // zero-fill
        std::fill_n(_mem, _capacity, memblk{});
    }

    explicit basic_queue_buffer(size_t capacity)
            : basic_queue_buffer(capacity, Alloc_{}) {}

    basic_queue_buffer() = default;

    basic_queue_buffer(basic_queue_buffer&& other) noexcept
    {
        _assign_norelease(std::move(other));
    }

    ~basic_queue_buffer() { _mem && (_allocator.deallocate(_mem), _mem = nullptr); }

    basic_queue_buffer& operator=(basic_queue_buffer&& other)
    {
        ~basic_queue_buffer();
        _assign_norelease(std::move(other));
        return *this;
    }

    void* allocate(size_t n)
    {
        return _alloc(n);
    }

    void deallcoate(void* p) noexcept
    {
        _dealloc(p);
    }

    void pop() noexcept
    {
        _dealloc(front());
    }

    void* front() noexcept
    {
        return (void*)(_tail + 1);
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

   private:
    void _assign_norelease(basic_queue_buffer&& other)
    {
        _allocator = std::move(other._allocator);
        _capacity  = other._capacity;
        _mem       = other._mem;

        other._capacity = 0;
        other._mem      = nullptr;
    }

   private:
    void* _alloc(size_t n)
    {
        if (n == 0)
            throw std::invalid_argument{"bad allocation size"};

        auto num_block = _size_in_blocks(n);

        if (not _head)
        {
            if (num_block + 1 > _capacity)
                throw queue_out_of_memory{};

            _head = _tail = _refer(0);
            _head->size   = num_block;
        }
        else
        {
            // 1.   if there's not enough space to allocate new memory block,
            //       fill tail memory to end, and insert new item at first.
            // 2.   otherwise, simply allocate new item at candidate place.

            auto candidate = _head + _head->size + 1;

            if (_head >= _tail)
            {
                if (candidate + num_block >= _border())
                {
                    _head->size = _border() - _head - 1;
                    candidate   = _refer(0);

                    if (candidate + 1 + num_block >= _tail)
                        throw queue_out_of_memory{};
                }

                if (candidate->occupied())
                    throw queue_out_of_memory{};
            }
            else
            {
                if (candidate + num_block >= _tail)
                    throw queue_out_of_memory{};

                if (candidate->occupied())
                    throw queue_out_of_memory{};
            }

            candidate->size = num_block;
            _head           = candidate;
        }

        ++_num_alloc;
        return _head + 1;
    }

    void _dealloc(void* p) noexcept
    {
        assert(p && not empty());

        if (auto* _node = (memblk*)p - 1; _node != _tail)
        {
            _node->defferred = true;
        }
        else
        {
            do  // perform all deferred deallocations
            {
                auto next = _next(_tail);
                *_tail    = {};
                _tail     = next;

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

    constexpr static size_t _size_in_blocks(uint32_t n) noexcept
    {
        return ((n + block_size - 1) / block_size);
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
    Alloc_ _allocator = {};
    size_t _capacity  = 0;
    size_t _num_alloc = 0;
    memblk* _mem      = nullptr;
    memblk* _tail     = nullptr;  // defined as forward-list
    memblk* _head     = nullptr;
};

using queue_buffer = basic_queue_buffer<>;

}  // namespace CPPHEADERS_NS_