#pragma once
#include <type_traits>

#if __has_include("../__cppheaders_ns__.h")
#include "../__cppheaders_ns__.h"
#else
namespace KANGSW_TEMPLATE_NAMESPACE
#endif
{
  template <typename Callable_>
  class cleanup {
   public:
    cleanup(Callable_&& callable) : _callable(std::move(callable)) {}
    ~cleanup() noexcept(std::is_nothrow_invocable_v<Callable_>) { _callable(); }

   private:
    Callable_ _callable;
  };
}  // namespace KANGSW_TEMPLATE_NAMESPACE