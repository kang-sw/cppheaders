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
 *   event_queue::dispatcher dp{s};
 *
 *   // producer
 *   dp.message(
 *      [&](event_queue::dispatcher::proxy p) {
 *        p.strand({572});   // events that have same strand will always be invoked sequentially.
 *                           // otherwise, its sequentiality is not guaranteed.
 *                           // strand is initialized with 0 in default, which can always be
 *                           //  invoked in parallel context(default).
 *
 *        p.function(
 *          [](array_view<int> param1, std::string&& param2, double param3) {
 *            // do some operation
 *          });
 *
 *        // must be ordered correctly, and
 *        auto v = p.param<int[]>(1611);
 *        std::iota(v.begin(), v.end(), 0);
 *
 *        auto& str = p.param<std::string>();
 *        str.append("vlvlv");
 *
 *        p.param<double>() = 6.11;
 *      }
 *   );
 *
 *   // consumer has various context data to perform optimized strand sort, etc ...
 *   // consumer must be created per-thread if in concurrency context.
 *   event_consumer ed {event_queue};
 *   ed.consume_one();
 *   ed.consume();
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
using strand_t = basic_key<class LABEL_node_strand_t>;

}  // namespace CPPHEADERS_NS_::event_queue

struct parameter_size_mismatch_exception : std::exception {
  size_t specified;
  size_t desired;
};

class context {
};

// proxy class of context
class dispatcher {
  class proxy {

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
  strand_t strand               = {};
  std::function<void()> event_fn;
  node_parameter_t* root_param = {};  // parameter node root

  uint8_t occupation = 1;  // number of total occupied queue_allocator node
  uint8_t num_params = 0;  // number of allcoated parameters
};

}  // namespace detail
#endif
