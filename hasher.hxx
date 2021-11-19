#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "type_traits.hxx"

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_ {
namespace hasher {
constexpr static uint64_t FNV_PRIME       = 0x100000001b3ull;
constexpr static uint64_t FNV_OFFSET_BASE = 0xcbf29ce484222325ull;

/// hash a single byte
constexpr inline uint64_t fnv1a_byte(unsigned char byte, uint64_t hash) {
  return (hash ^ byte) * FNV_PRIME;
}

template <typename Ty_>
constexpr inline uint64_t fnv1a_64(Ty_ const& val, uint64_t base = FNV_OFFSET_BASE);

template <typename It_>
constexpr inline uint64_t fnv1a_64(It_ begin, It_&& end, uint64_t base = FNV_OFFSET_BASE) {
  for (; begin != end; ++begin) {
    if constexpr (sizeof(*begin) == 1)
      base = fnv1a_byte(*begin, base);
    else
      base = fnv1a_64(*begin, base);
  }
  return base;
}

template <typename T>
using _is_range_t = decltype(std::begin(std::declval<T>()));

template <typename Ty_>
constexpr inline uint64_t fnv1a_64(Ty_ const& val, uint64_t base) {
  if constexpr (type_traits::is_detected_v<_is_range_t, Ty_>) {
    return fnv1a_64(std::begin(val), std::end(val));
  } else if constexpr (std::is_trivial_v<Ty_>) {
    return fnv1a_64((char const*)&val, (char const*)(&val + 1), base);
  } else {
    val.GENERATE_STATIC_ASSERT();
  }
}
}  // namespace hasher

template <typename Label_>
struct key_base {
  bool operator<(key_base const& other) const noexcept { return value < other.value; }
  bool operator==(key_base const& other) const noexcept { return value == other.value; }

  template <typename Other_,
            typename = std::enable_if_t<std::is_arithmetic_v<Other_>>>
  friend bool operator<(Other_&& v, key_base const& self) noexcept { return v < self.value; }

  template <typename Other_,
            typename = std::enable_if_t<std::is_arithmetic_v<Other_>>>
  friend bool operator<(key_base const& self, Other_&& v) noexcept { return self.value < v; }

  operator bool() const noexcept { return value != 0; }

 public:
  uint64_t value = {};
};

}  // namespace CPPHEADERS_NS_

namespace std {
template <typename Label_>
struct hash<CPPHEADERS_NS_::key_base<Label_>> {
  auto operator()(CPPHEADERS_NS_::key_base<Label_> const& s) const noexcept {
    return ::std::hash<uint64_t>{}(s.value);
  }
};
}  // namespace std
