/**
 * Event queue. All operations are thread safe, as long as mutex is supplied.
 *
 * \see https://stackoverflow.com/questions/8224648/retrieve-function-arguments-as-a-tuple-in-c
 *
 * \code
 *   event_queue::context s{
 *      NUM_ALLOC_NODES,
 *      MAX_BUFFER_BYTES,
 *      CONCURRENCY_HINT // number of consumer context size. default is 1
 *   };
 *   // node limit, memory limit.
 *   // (1 node + 1 node per parameter) per message
 *
 *
 *   // produce messgae.
 *   // .message() to commit(), is critical section.
 *   // if proxy destroyed before commit(), it'll throw.
 *   auto msg_id = s.message()  // := context::message_build_proxy<>
 *      .arg<int[]>(1024, [&](array_view<int>&) {}) // := context::message_build_proxy<array<int,1024>>
 *      .arg<std::string>("hello, world!") // := context::message_build_proxy<array<int,1024>, std::string>
 *      .strand(strand_key_t{1211})
 *      .commit([](array_view<int>, std::string&&) {}) // register handler.
 *
 *   s.message()
 *      ...
 *      .after(msg_id) // can control invocation flow
 *      ...
 *      .commit(...);
 *
 *   // otherwise, an allocate() call can be made to escape critical section
 *   //  as fast as possible. with allocate() call, proxy remains valid, thus you can
 *   //  call handle() method separately.
 *   // However, strand() call must be made before allocate(). (within critical section)
 *   // as .param<>() call
 *   s.message()
 *      .strand({1944})
 *      .arg(&s) // can deduce from parameter
 *      .arg<char[]>(564)
 *      .arg<std::string>("hello!")
 *      .arg<int>(564)
 *      .arg<std::future>(std::async(sqrt, 3.14)) // full_proxy<...>
 *      .allocate()                               // escaped_proxy<...>
 *      .modify<0>([](array_view<char>) {})
 *      .modify([](array_view<char>
 *      .commit(...)
 *
 *   s.consume_one();
 *   s.consume(); // run until empty
 *   s.consume_for(100ms);
 *   s.consume_until(steady_clock::now() + 591ms);
 *
 *   // abort all deferred consume operations
 *   s.abort();
 * \endcode
 */
#pragma once

#include "array_view.hxx"
#include "hasher.hxx"
#include "memory/queue_allocator.hxx"

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_::task_queue
{
using strand_key_t = basic_key<class LABEL_node_strand_t>;

}  // namespace CPPHEADERS_NS_::task_queue

struct parameter_size_mismatch_exception : std::exception
{
    size_t specified;
    size_t desired;
};

class context
{
    class proxy
    {
       public:
        void strand(event_queue::strand_t group_key);

        template <typename Fn_>
        void function(Fn_&& callable);
    };

    // proxy class of context consumer logic
    // context는 컨슈머 참조 리스트를 물고, 새 이벤트가 commit 될 때마다 비는 컨슈머를 찾아 이벤트 꽂고
    //  notify 날린다. 이 때 배열은 round-robin 방식으로 순환. 각 consumer 인스턴스의
    class consumer
    {
    };
};

}  // namespace CPPHEADERS_NS_::event_queue

#if !defined(CPPHEADERS_ENABLE_CONFIGURE) || defined(CPPHEADERS_IMPLEMENT_EVENT_QUEUE)
#    include <atomic>
#    include <condition_variable>
#    include <functional>
#    include <mutex>

namespace CPPHEADERS_NS_::event_queue
{
namespace detail
{
enum class node_state
{
    uninitialized,
    allocated,       // being built. finish_allocation() is called.
    committed,       // commit() is called.
    running,         // being invocated
    erasing,         // got into erasing sequence.
    erase_deferred,  // this node wasn't foremost node when it finished invocation.
    MAX
};

struct node_parameter_t
{
    void* data;
    node_parameter_t* next = {};
};

struct node_t
{
    int64_t fence = 0;

    std::atomic<node_state> state = {};
    strand_key_t strand           = {};
    node_parameter_t* root_param  = {};  // parameter node root

    uint8_t occupation = 1;  // number of total occupied queue_allocator node
    uint8_t num_params = 0;  // number of all allcoated parameters

    std::function<void()> event_fn;
};

}  // namespace detail
#endif
