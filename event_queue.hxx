/**
 * Event queue. All operations are thread safe, as long as mutex is supplied.
 *
 * \see https://stackoverflow.com/questions/8224648/retrieve-function-arguments-as-a-tuple-in-c
 *
 * \code
 *   event_queue::context s; // node limit, memory limit.
 *                             // (1 node + 1 node per parameter) per message
 *
 *
 *   // producer
 *   auto msg_id = s.message()  // := context::message_build_proxy<>
 *      .param<int[]>(1024, [&](array_view<int>&) {}) // := context::message_build_proxy<array<int,1024>>
 *      .param<std::string>("hello, world!") // := context::message_build_proxy<array<int,1024>, std::string>
 *      .strand(strand_key_t{1211})
 *      .preprocess([](array_view<int>&, std::string&) {})
 *      .commit([](array_view<int>, std::string&&) {}) // register handler.
 *
 *   s.message()
 *    ...
 *    .after(msg_id) // can control invocation flow
 *    ...
 *    .commit(...);
 *
 *   // consumer has various context data to perform optimized strand sort, etc ...
 *   // consumer must be created per-thread if in concurrency context.
 *   event_consumer ed {event_queue};
 *   ed.consume_one();
 *   ed.consume(); // run until empty
 *   ed.consume_for(100ms);
 *   ed.consume_until(steady_clock::now() + 591ms);
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

namespace CPPHEADERS_NS_::event_queue {
using strand_key_t = basic_key<class LABEL_node_strand_t>;

}  // namespace CPPHEADERS_NS_::event_queue

struct parameter_size_mismatch_exception : std::exception {
  size_t specified;
  size_t desired;
};

class context {
  class proxy {
   public:
    void strand(event_queue::strand_t group_key);

    template <typename Fn_>
    void function(Fn_&& callable);
  };
};

// proxy class of consumer
class consumer {
};

}  // namespace CPPHEADERS_NS_::event_queue

#if !defined(CPPHEADERS_ENABLE_CONFIGURE) || defined(CPPHEADERS_IMPLEMENT_EVENT_QUEUE)
#  include <atomic>
#  include <condition_variable>
#  include <functional>
#  include <mutex>

namespace CPPHEADERS_NS_::event_queue {
namespace detail {
enum class node_state {
  uninitialized,
  building,
  waiting,
  occupied,
  erasing,         // got into erasing sequence.
  erase_deferred,  // this node wasn't foremost node when it finished invocation.
  MAX
};

struct node_parameter_t {
  void* data;
  node_parameter_t* next = {};
};

struct node_t {
  int64_t fence = 0;

  std::atomic<node_state> state = {};
  strand_key_t strand           = {};
  std::function<void()> event_fn;
  node_parameter_t* root_param = {};  // parameter node root

  uint8_t occupation = 1;  // number of total occupied queue_allocator node
  uint8_t num_params = 0;  // number of allcoated parameters
};

}  // namespace detail
#endif
