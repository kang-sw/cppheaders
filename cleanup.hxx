#pragma once
#include <type_traits>

#include "__namespace__.h"

namespace CPPHEADERS_NS_ {
template <typename Callable_>
class cleanup {
 public:
  cleanup(Callable_&& callable) : _callable(std::move(callable)) {}
  ~cleanup() noexcept(std::is_nothrow_invocable_v<Callable_>) { _callable(); }

 private:
  Callable_ _callable;
};
}  // namespace CPPHEADERS_NS_