/**
 * allocates byte array sequentially.
 * \code
 *   basic_queue_allocator<std::allocator<char>> alloc {1024};
 *
 *   // alloc.resize(); queue allocator doesn't provide way to resize buffer in runtime
 *   //                 if size adjustment is required, a new allocator must be assigned.
 *   alloc = basic_queue_allocator<std::allocator>{2048};
 *
 *   // template<typename Ty_> Ty_& ..::push(Ty_&&);
 *   alloc.push(3);
 *
 *   // template<typename Ty_, typename... Args_>
 *   // Ty_& ..::emplace(Args_...);
 *   alloc.emplace<std::string>("hell, world!");
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
 *   // allocate array
 *   // in this case,
 *   auto k = alloc.push_array<int>(1024);
 *   // k := array_view<int>; k.size() == 1024;
 *
 *   // ~basic_queue_allocator() -> will release all allocations automatically.
 * \endcode
 */