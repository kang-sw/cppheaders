
// MIT License
//
// Copyright (c) 2021-2022. Seungwoo Kang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// project home: https://github.com/perfkitpp

#pragma once
#include <algorithm>
#include <numeric>

//
#include "../__namespace__"

namespace CPPHEADERS_NS_ {
inline namespace algorithm {
#define INTERNAL_CPPH_DEFINE_WRAPPER(FUNC)                                          \
    template <typename Container_, typename... Args_>                               \
    auto FUNC(Container_&& a, Args_&&... args)                                      \
    {                                                                               \
        return std::FUNC(std::begin(a), std::end(a), std::forward<Args_>(args)...); \
    }

#define INTERNAL_CPPH_DEFINE_WRAPPER2(FUNC)                                  \
    template <typename ContainerA_, typename ContainerB_, typename... Args_> \
    auto FUNC##2(ContainerA_ && a, ContainerB_ && b, Args_ && ... args)      \
    {                                                                        \
        return std::FUNC(                                                    \
                std::begin(a), std::end(a),                                  \
                std::begin(b), std::end(b),                                  \
                std::forward<Args_>(args)...);                               \
    }

INTERNAL_CPPH_DEFINE_WRAPPER(all_of)
INTERNAL_CPPH_DEFINE_WRAPPER(any_of)
INTERNAL_CPPH_DEFINE_WRAPPER(none_of)

INTERNAL_CPPH_DEFINE_WRAPPER(for_each)
INTERNAL_CPPH_DEFINE_WRAPPER(for_each_n)

INTERNAL_CPPH_DEFINE_WRAPPER(count)
INTERNAL_CPPH_DEFINE_WRAPPER(count_if)

INTERNAL_CPPH_DEFINE_WRAPPER(mismatch)

INTERNAL_CPPH_DEFINE_WRAPPER(find)
INTERNAL_CPPH_DEFINE_WRAPPER(find_if)
INTERNAL_CPPH_DEFINE_WRAPPER(find_if_not)

INTERNAL_CPPH_DEFINE_WRAPPER(find_end)
INTERNAL_CPPH_DEFINE_WRAPPER(find_first_of)
INTERNAL_CPPH_DEFINE_WRAPPER(adjacent_find)
INTERNAL_CPPH_DEFINE_WRAPPER(search)
INTERNAL_CPPH_DEFINE_WRAPPER(search_n)

INTERNAL_CPPH_DEFINE_WRAPPER(copy)
INTERNAL_CPPH_DEFINE_WRAPPER(copy_if)
INTERNAL_CPPH_DEFINE_WRAPPER(copy_n)
INTERNAL_CPPH_DEFINE_WRAPPER(copy_backward)

INTERNAL_CPPH_DEFINE_WRAPPER(move)
INTERNAL_CPPH_DEFINE_WRAPPER(move_backward)

INTERNAL_CPPH_DEFINE_WRAPPER(fill)
INTERNAL_CPPH_DEFINE_WRAPPER(fill_n)

INTERNAL_CPPH_DEFINE_WRAPPER(transform)
INTERNAL_CPPH_DEFINE_WRAPPER(generate)
INTERNAL_CPPH_DEFINE_WRAPPER(generate_n)

INTERNAL_CPPH_DEFINE_WRAPPER(remove)
INTERNAL_CPPH_DEFINE_WRAPPER(remove_if)
INTERNAL_CPPH_DEFINE_WRAPPER(remove_copy)
INTERNAL_CPPH_DEFINE_WRAPPER(remove_copy_if)

INTERNAL_CPPH_DEFINE_WRAPPER(replace)
INTERNAL_CPPH_DEFINE_WRAPPER(replace_if)
INTERNAL_CPPH_DEFINE_WRAPPER(replace_copy)
INTERNAL_CPPH_DEFINE_WRAPPER(replace_copy_if)

INTERNAL_CPPH_DEFINE_WRAPPER(swap_ranges)
INTERNAL_CPPH_DEFINE_WRAPPER(reverse)
INTERNAL_CPPH_DEFINE_WRAPPER(reverse_copy)
INTERNAL_CPPH_DEFINE_WRAPPER(rotate)
INTERNAL_CPPH_DEFINE_WRAPPER(rotate_copy)

INTERNAL_CPPH_DEFINE_WRAPPER(shuffle)
INTERNAL_CPPH_DEFINE_WRAPPER(sample)
INTERNAL_CPPH_DEFINE_WRAPPER(unique)
INTERNAL_CPPH_DEFINE_WRAPPER(unique_copy)

INTERNAL_CPPH_DEFINE_WRAPPER(is_partitioned)
INTERNAL_CPPH_DEFINE_WRAPPER(partition)
INTERNAL_CPPH_DEFINE_WRAPPER(partition_copy)
INTERNAL_CPPH_DEFINE_WRAPPER(stable_partition)
INTERNAL_CPPH_DEFINE_WRAPPER(partition_point)

INTERNAL_CPPH_DEFINE_WRAPPER(is_sorted)
INTERNAL_CPPH_DEFINE_WRAPPER(is_sorted_until)
INTERNAL_CPPH_DEFINE_WRAPPER(sort)
INTERNAL_CPPH_DEFINE_WRAPPER(partial_sort)
INTERNAL_CPPH_DEFINE_WRAPPER(partial_sort_copy)
INTERNAL_CPPH_DEFINE_WRAPPER(stable_sort)
INTERNAL_CPPH_DEFINE_WRAPPER(nth_element)

INTERNAL_CPPH_DEFINE_WRAPPER(lower_bound)
INTERNAL_CPPH_DEFINE_WRAPPER(upper_bound)
INTERNAL_CPPH_DEFINE_WRAPPER(binary_search)
INTERNAL_CPPH_DEFINE_WRAPPER(equal_range)

INTERNAL_CPPH_DEFINE_WRAPPER(merge)
INTERNAL_CPPH_DEFINE_WRAPPER(inplace_merge)

INTERNAL_CPPH_DEFINE_WRAPPER(includes)
INTERNAL_CPPH_DEFINE_WRAPPER2(includes)
INTERNAL_CPPH_DEFINE_WRAPPER(set_difference)
INTERNAL_CPPH_DEFINE_WRAPPER2(set_difference)
INTERNAL_CPPH_DEFINE_WRAPPER(set_intersection)
INTERNAL_CPPH_DEFINE_WRAPPER2(set_intersection)
INTERNAL_CPPH_DEFINE_WRAPPER(set_symmetric_difference)
INTERNAL_CPPH_DEFINE_WRAPPER2(set_symmetric_difference)
INTERNAL_CPPH_DEFINE_WRAPPER(set_union)
INTERNAL_CPPH_DEFINE_WRAPPER2(set_union)

INTERNAL_CPPH_DEFINE_WRAPPER(is_heap)
INTERNAL_CPPH_DEFINE_WRAPPER(is_heap_until)
INTERNAL_CPPH_DEFINE_WRAPPER(make_heap)
INTERNAL_CPPH_DEFINE_WRAPPER(push_heap)
INTERNAL_CPPH_DEFINE_WRAPPER(pop_heap)
INTERNAL_CPPH_DEFINE_WRAPPER(sort_heap)

INTERNAL_CPPH_DEFINE_WRAPPER(max)
INTERNAL_CPPH_DEFINE_WRAPPER(max_element)
INTERNAL_CPPH_DEFINE_WRAPPER(min)
INTERNAL_CPPH_DEFINE_WRAPPER(min_element)

INTERNAL_CPPH_DEFINE_WRAPPER(minmax)
INTERNAL_CPPH_DEFINE_WRAPPER(minmax_element)

INTERNAL_CPPH_DEFINE_WRAPPER(equal)
INTERNAL_CPPH_DEFINE_WRAPPER2(equal)
INTERNAL_CPPH_DEFINE_WRAPPER(lexicographical_compare)
INTERNAL_CPPH_DEFINE_WRAPPER2(lexicographical_compare)

INTERNAL_CPPH_DEFINE_WRAPPER(is_permutation)
INTERNAL_CPPH_DEFINE_WRAPPER2(is_permutation)
INTERNAL_CPPH_DEFINE_WRAPPER(next_permutation)
INTERNAL_CPPH_DEFINE_WRAPPER(prev_permutation)

INTERNAL_CPPH_DEFINE_WRAPPER(iota)
INTERNAL_CPPH_DEFINE_WRAPPER(accumulate)
INTERNAL_CPPH_DEFINE_WRAPPER(inner_product)
INTERNAL_CPPH_DEFINE_WRAPPER(adjacent_difference)
INTERNAL_CPPH_DEFINE_WRAPPER(partial_sum)
INTERNAL_CPPH_DEFINE_WRAPPER(reduce)
INTERNAL_CPPH_DEFINE_WRAPPER(exclusive_scan)
INTERNAL_CPPH_DEFINE_WRAPPER(inclusive_scan)
INTERNAL_CPPH_DEFINE_WRAPPER(transform_reduce)
INTERNAL_CPPH_DEFINE_WRAPPER(transform_exclusive_scan)
INTERNAL_CPPH_DEFINE_WRAPPER(transform_inclusive_scan)

#undef INTERNAL_CPPH_DEFINE_WRAPPER

// helper methods
template <typename Range_, typename Pred_>
auto erase_if(Range_&& range, Pred_&& pred)
{
    auto iter = std::remove_if(std::begin(range), std::end(range), std::forward<Pred_>(pred));
    return range.erase(iter, std::end(range));
}

template <typename Range_, typename Pred_>
size_t erase_if_each(Range_&& map, Pred_&& pred)
{
    size_t n_erased = 0;

    for (auto it = map.begin(); it != map.end();)
    {
        if (pred(*it))
            it = map.erase(it), ++n_erased;
        else
            ++it;
    }

    return n_erased;
}

template <typename Set_, typename Key_>
auto find_ptr(Set_&& set, Key_ const& key)
        -> decltype(&*set.find(key))
{
    auto it = set.find(key);
    if (it == set.end())
    {
        return nullptr;
    }
    else
    {
        return &*it;
    }
}

template <typename Float_>
auto range_alpha(Float_ value, Float_ v1, Float_ v2)
{
    if (v1 == v2) { return Float_(value > v1); }
    auto [min, max] = std::minmax(v1, v2);

    Float_ alpha = std::clamp(value, min, max);
    alpha        = (alpha - min) / (max - min);

    return alpha;
}

template <typename Alpha_, typename ValTy_>
auto lerp(ValTy_ const& a, ValTy_ const& b, Alpha_ const& alpha)
{
    return a + (b - a) * alpha;
}

template <typename ValTy_ = double, typename Iter_, typename Mult_ = std::multiplies<>>
auto variance(Iter_ begin, Iter_ end, Mult_ mult = std::multiplies<>{})
{
    auto div  = static_cast<ValTy_>(std::distance(begin, end));
    auto mean = std::reduce(begin, end) / div;
    return std::transform_reduce(
                   begin, end,
                   decltype(mean){},
                   std::plus<>{},
                   [&](auto&& v) {
                       auto s = v - mean;
                       return mult(s, s);
                   })
         / div;
}

template <typename ValTy_ = double, typename Container_, typename Mult_ = std::multiplies<>>
auto variance(Container_&& cont, Mult_ mult = std::multiplies<>{})
{
    return variance(std::begin(cont), std::end(cont), std::forward<Mult_>(mult));
}

struct owner_equal_t
{
    template <typename PtrA_, typename PtrB_>
    bool operator()(PtrA_ const& a, PtrB_ const& b) const noexcept
    {
        return not a.owner_before(b) && not b.owner_before(a);
    }
};
constexpr owner_equal_t owner_equal{};

}  // namespace algorithm
};  // namespace CPPHEADERS_NS_

namespace std {
inline namespace literals {
}
}  // namespace std

namespace CPPHEADERS_NS_::util_ns {
using namespace std::literals;
}
