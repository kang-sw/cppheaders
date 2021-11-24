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
 *   // basic_context<MutexType_, AllocType_>
 *
 *   auto stride = s.create_stride(); // assign random stride
 *   auto stride_2 = s.stride({16151}); // refer to other stride
 *
 *   // option 1
 *   s.message(stride) // declare new message. no operation yet submitted
 *     .param<int[], double, std::string>() // declare parameters. not yet copied.
 *     .handler([=](std::array_view<int>, double&, std::string&) {}) // will be move-copied
 *     .allocate() // allocate memory for declared parameter/handlers
 *     .emplace<0>(16141) // initializes first parameter(int[]) with size 16141
 *     .construct<1>([&](double& s) { s = 3.111; }); // modify default-constructed data
 *     .construct<2>("hell!", [&](std::string& s) { s = "world!"; }) // construct first, then modify
 *     .commit(); // signal ready-to-execute
 *
 *   // option 2
 *   // do everything at once.
 *   s.message(stride_2)
 *     .commit(my_functor{this, capture1}, param1, param2, param3);
 *
 *   // as asnyc task ...
 *   event_queue::future<double> fut = s.message() // alias of std::future.
 *     .commit([] { return some_long_calculation(); });
 *
 *   // note: using char[] instead of std::string is recommended, to take maximum
 *   //  advantage of queue buffer
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
