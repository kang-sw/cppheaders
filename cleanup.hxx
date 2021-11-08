#pragma once
#include <type_traits>

#include "__namespace__.h"
{
  template <typename Callable_>
  class cleanup {
   public:
    cleanup(Callable_&& callable) : _callable(std::move(callable)) {}
    ~cleanup() noexcept(std::is_nothrow_invocable_v<Callable_>) { _callable(); }

   private:
    Callable_ _callable;
  };
}