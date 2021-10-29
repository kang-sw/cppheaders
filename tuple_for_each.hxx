#pragma once
#include <tuple>

namespace KANGSW_TEMPLATE_NAMESPACE  //
{

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
}  // namespace KANGSW_TEMPLATE_NAMESPACE