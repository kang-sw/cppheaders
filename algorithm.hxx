/**
 * STL algorithm wrapper for containers
 */

#pragma once
#include <algorithm>
#include <numeric>

//
#include "__namespace__.h"

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

}  // namespace algorithm
}  // namespace CPPHEADERS_NS_