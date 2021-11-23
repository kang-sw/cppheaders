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
#include <cstddef>
#include <cstdint>
#include <utility>

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_ {
namespace detail {
struct queue_buffer_block {
  uint32_t defferred : 1;
  uint32_t offset    : 31;  // units are in sizeof(_node_type)

  // there's no field to indicate next pos.
  // ((_blk_type*)_mem + size) will point next memory block header.
  // if offset + size exceeds capacity, it implicitly means its next node is _refer(0).
  uint32_t size;
};

static_assert(sizeof(queue_buffer_block) == 8);
}  // namespace detail

template <class Alloc_>
class basic_queue_buffer {
 public:
  using allocator_type = Alloc_;
  enum { block_size = 8 };

 private:
  using blk_type = detail::queue_buffer_block;

 public:
  template <typename OtherAlloc_>
  basic_queue_buffer(size_t capacity, OtherAlloc_&& allocator)
          : _allocator{std::forward<OtherAlloc_>(allocator)},
            _capacity{_aligned_size(capacity)},
            _mem{_allocator.allocate(capacity * block_size)} {}

  basic_queue_buffer(size_t capacity)
          : basic_queue_buffer(capacity, {}) {}

  basic_queue_buffer(basic_queue_buffer&& other) {
    _assign_norelease(std::move(other));
  }

  ~basic_queue_buffer() { _mem && (_allocator.deallocate(_mem), _mem = nullptr); }

  basic_queue_buffer& operator=(basic_queue_buffer&& other) {
    ~basic_queue_buffer();
    _assign_norelease(std::move(other));
    return *this;
  }

  void* allocate(size_t n) {
    return _alloc(n);
  }

  void deallcoate(void* p) noexcept {
    _dealloc(p);
  }

 private:
  void _assign_norelease(basic_queue_buffer&& other) {
    _allocator = std::move(other._allocator);
    _capacity  = other._capacity;
    _mem       = other._mem;

    other._capacity = 0;
    other._mem      = nullptr;
  }

 private:
  void* _alloc(size_t n) {
    if (not _tail) {
      _tail = _head = _create_at(0, n);
    } else {
      // TODO: update tail as appropriate new node.
    }
    return _tail + 1;
  }

  void _dealloc(void* p) noexcept {
    auto* _node = (blk_type*)p - 1;
    if (_node != _head) {
      _node->defferred = true;
    }
  }

  blk_type* _create_at(uint32_t offset, uint32_t size) noexcept {
    auto* node      = _refer(offset);
    node->offset    = offset;
    node->defferred = 0;
    node->size      = _aligned_size(size);
    return node;
  }

  blk_type* _refer(uint32_t offset) noexcept {
    return &((blk_type*)_mem)[offset];
  }

  constexpr static size_t _aligned_size(uint32_t n) noexcept {
    return ((n + block_size - 1) / block_size);
  }

 private:
  Alloc_ _allocator;
  size_t _capacity;
  void* _mem;
  blk_type* _head = nullptr;  // defined as forward-list
  blk_type* _tail = nullptr;
};

}  // namespace CPPHEADERS_NS_