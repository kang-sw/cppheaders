/**
 * Event queue. All operations are thread safe, as long as mutex is supplied.
 *
 * \see https://stackoverflow.com/questions/8224648/retrieve-function-arguments-as-a-tuple-in-c
 *
 * \code
 *   using event_queue = basic_event_queue<std::mutex, std::allocator<char>>;
 *   event_queue s{4096}; // memory limit.
 *
 *
 *   // producer
 *   s.message(
 *      [&](event_queue::message_proxy p) {
 *        p.function(
 *          [](int param1, std::string&& param2, double param3) {
 *            // do some operation
 *          });
 *
 *        // must be ordered correctly, and
 *        p.param<int>(1);
 *
 *        auto& str = p.param<std::string>();
 *        str.append("vlvlv");
 *
 *        p.param<double>() = 6.11;
 *      }
 *   );
 *
 *   // consumer
 *   s.consume_one();
 *   s.consume();
 *   s.consume_for(100ms);
 *   s.consume_until(steady_clock::now() + 591ms);
 *
 * \endcode
 */