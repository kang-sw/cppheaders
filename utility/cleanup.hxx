#pragma once
#include <type_traits>
#include <utility>

#include "../__namespace__.h"

namespace CPPHEADERS_NS_ {
template <typename Callable_>
class _cleanup_t
{
   public:
    ~_cleanup_t() noexcept(std::is_nothrow_invocable_v<Callable_>) { _callable(); }
    Callable_ _callable;
};

template <typename Callable_>
auto cleanup(Callable_&& callable)
{
    return _cleanup_t<std::remove_reference_t<Callable_>>{
            std::forward<Callable_>(callable)};
}
}  // namespace CPPHEADERS_NS_