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