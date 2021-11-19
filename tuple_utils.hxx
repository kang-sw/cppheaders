#pragma once
#include <tuple>

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_ {
/**
 * @see https://stackoverflow.com/questions/1198260/how-can-you-iterate-over-the-elements-of-an-stdtuple
 * Iterate over tempalte
 */
template <std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
tuple_for_each(std::tuple<Tp...>&, FuncT&&)  // Unused arguments are given no names.
{}

template <std::size_t I = 0, typename FuncT, typename... Tp>
        inline typename std::enable_if < I<sizeof...(Tp), void>::type
        tuple_for_each(std::tuple<Tp...>& t, FuncT&& f) {
  if constexpr (std::is_invocable_v<FuncT, decltype(std::get<I>(t)), size_t>) {
    f(std::get<I>(t), I);
  } else {
    f(std::get<I>(t));
  }
  tuple_for_each<I + 1, FuncT&&, Tp...>(t, std::forward<FuncT>(f));
}

template <std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
tuple_for_each(std::tuple<Tp...> const&, FuncT&&)  // Unused arguments are given no names.
{}

template <std::size_t I = 0, typename FuncT, typename... Tp>
        inline typename std::enable_if < I<sizeof...(Tp), void>::type
        tuple_for_each(std::tuple<Tp...> const& t, FuncT&& f) {
  if constexpr (std::is_invocable_v<FuncT, decltype(std::get<I>(t)), size_t>) {
    f(std::get<I>(t), I);
  } else {
    f(std::get<I>(t));
  }
  tuple_for_each<I + 1, FuncT&&, Tp...>(t, std::forward<FuncT>(f));
}

/**
 * wrapper for borrowing ranges
 */
template <typename Begin_, typename End_>
struct _borrowed_range {
  Begin_ _begin;
  End_ _end;

  auto begin() const noexcept { return _begin; }
  auto end() const noexcept { return _end; }

  auto begin() noexcept { return _begin; }
  auto end() noexcept { return _end; }
};

template <typename Begin_, typename End_>
auto make_iterable(Begin_&& begin, End_&& end) -> _borrowed_range<Begin_, End_> {
  return {std::forward<Begin_>(begin), std::forward<End_>(end)};
}


}  // namespace CPPHEADERS_NS_