#pragma once
#include <atomic>

#include "spinlock.hxx"

/**
 * handle in circular queue:
 *
 *  As internal memory of circular queue is totally reallocated when internal buffer is full,
 */
//
#if __has_include("../__cppheaders_ns__.h")
#include "../__cppheaders_ns__.h"
#else
namespace KANGSW_TEMPLATE_NAMESPACE
#endif
::circular_buffer {
  class allocator;

  class _ptr_base {
   public:
    _ptr_base(allocator* owner = {});
    ~_ptr_base();
  };

  template <typename Ty_>
  class _ptr : private _ptr_base {
   public:
    Ty_* get() noexcept;
    Ty_& operator*() noexcept;
    Ty_* operator->() noexcept;
  };

  template <typename Ty_>
  class handle {
   public:
    _ptr<Ty_> lock() noexcept;
    void reset();

   private:
    allocator* _owner;
    size_t _ofst;
  };

  /**
   * Features:
   *
   * - FIFO memory allocations for arbitrary types
   * - Safe memory management via reference counter
   */
  class allocator {
    struct alignas(64) memblk {
      size_t offset;
      size_t next_offset;
      void (*release)(void*);

      char memory[0];

      memblk* next(void* buf);
    };

   public:
    handle<void> allocate(size_t byte);

    template <typename Ty_, typename... Args_>
    handle<Ty_> enqueue(Args_&&... args);

   private:
   private:
    template <typename Ty_>
    friend class handle;

    std::unique_ptr<char[]> _buffer;
    memblk* _head;
  };

}  // namespace KANGSW_TEMPLATE_NAMESPACE::circular_buffer
