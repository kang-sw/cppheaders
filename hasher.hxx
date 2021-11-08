#pragma once
#include <cstddef>
#include <cstdint>
//
#include "__namespace__.h"
{
  namespace hasher {
  constexpr static uint64_t FNV_PRIME       = 0x100000001b3ull;
  constexpr static uint64_t FNV_OFFSET_BASE = 0xcbf29ce484222325ull;

  /// hash a single byte
  static constexpr inline uint64_t fnv1a_byte(unsigned char byte, uint64_t hash) {
    return (hash ^ byte) * FNV_PRIME;
  }

  template <typename It_>
  static constexpr inline uint64_t fnv1a_64(It_ begin, It_&& end, uint64_t base = FNV_OFFSET_BASE) {
    for (; begin != end; ++begin) { base = fnv1a_byte(*begin, base); }
    return base;
  }

  template <typename Ty_>
  static constexpr inline uint64_t fnv1a_64(Ty_ const& val, uint64_t base = FNV_OFFSET_BASE) {
    return fnv1a_64((char const*)&val, (char const*)(&val + 1), base);
  }

  template <typename Ch_, size_t N_>
  static constexpr inline uint64_t fnv1a_64(Ch_ (&buf)[N_], uint64_t base = FNV_OFFSET_BASE) {
    return fnv1a_64(buf, buf + N_, base);
  }
  }  // namespace hasher
}