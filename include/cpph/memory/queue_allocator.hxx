#pragma once
#include <cpph/std/list>
#include <cpph/std/vector>
#include <memory>

#include "cpph/thread/threading.hxx"
#include "cpph/utility/generic.hxx"
#include "pool.hxx"

namespace cpph {
namespace _detail {
struct queue_allocator_node {
    uint64_t occupied       : 1;
    uint64_t section_offset : 31;
    uint64_t block_count    : 32;
    char data[];
};

constexpr size_t queue_alloc_block_size = 16;
static_assert(sizeof(queue_allocator_node) <= queue_alloc_block_size);
}  // namespace _detail

template <class Mutex = null_mutex>
class queue_allocator : std::enable_shared_from_this<queue_allocator<Mutex>>
{
    using section_buffer_pool_type = vector<pair<ptr<char>, size_t>>;
    using node_type = _detail::queue_allocator_node;

   public:
    using mutex_type = Mutex;

   private:
    mutex_type mt_;
    section_buffer_pool_type pool_;
    vector<int> idle_pool_idxs_;  // List of available pools.

    node_type* first_;
    node_type* last_;

    size_t section_size_;

   public:
    explicit queue_allocator(size_t default_section_size) noexcept : section_size_(default_section_size) {}

   public:
    template <typename T>
    struct dtor_adaptor {
        using pointer = T*;
        sptr<queue_allocator<T>> d;
        void operator()(T* ptr) noexcept {}
    };

    template <typename T>
    using pointer_type = unique_ptr<T, dtor_adaptor<T>>;

   public:
    template <class T, class... Args>
    auto construct() -> pointer_type<T>;
};
}  // namespace cpph
