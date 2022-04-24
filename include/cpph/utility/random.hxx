#pragma once
#include <random>

#include "cpph/algorithm/std.hxx"
#include "cpph/template_utils.hxx"

namespace cpph {
template <typename OutIter, typename Generator>
OutIter generate_random_characters(OutIter o, size_t n, Generator&& gen)
{
    auto constexpr charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"sv;
    std::uniform_int_distribution<size_t> distr{0, size(charset) - 1};
    while (n--) { *o++ = charset[distr(gen)]; }
    return o;
}

template <typename OutIter>
OutIter generate_random_characters(OutIter o, size_t n)
{
    return generate_random_characters(o, n, std::mt19937_64{std::random_device{}()});
}
}  // namespace cpph