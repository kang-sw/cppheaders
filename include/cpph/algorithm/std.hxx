
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
#include <cassert>
#include <numeric>

#include "cpph/utility/templates.hxx"

//

namespace cpph {
inline namespace algorithm {
using std::move;

#define INTERNAL_CPPH_DEFINE_WRAPPER(FUNC)                                         \
    template <typename Container, typename... Args>                                \
    auto FUNC(Container&& a, Args&&... args)                                       \
    {                                                                              \
        return std::FUNC(std::begin(a), std::end(a), std::forward<Args>(args)...); \
    }

#define INTERNAL_CPPH_DEFINE_WRAPPER2(FUNC)                               \
    template <typename ContainerA, typename ContainerB, typename... Args> \
    auto FUNC##2(ContainerA && a, ContainerB && b, Args && ... args)      \
    {                                                                     \
        return std::FUNC(                                                 \
                std::begin(a), std::end(a),                               \
                std::begin(b), std::end(b),                               \
                std::forward<Args>(args)...);                             \
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

INTERNAL_CPPH_DEFINE_WRAPPER(max_element)
INTERNAL_CPPH_DEFINE_WRAPPER(min_element)
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
template <typename Range, typename Pred>
auto erase_if(Range&& range, Pred&& pred)
{
    auto iter = std::remove_if(std::begin(range), std::end(range), std::forward<Pred>(pred));
    return range.erase(iter, std::end(range));
}

template <typename Range, typename Pred>
size_t erase_if_each(Range&& map, Pred&& pred)
{
    size_t n_erased = 0;

    for (auto it = map.begin(); it != map.end();) {
        if (pred(*it))
            it = map.erase(it), ++n_erased;
        else
            ++it;
    }

    return n_erased;
}

template <class Range, class PredOp, class PredErase>
size_t for_each_or_erase(Range&& map, PredErase&& pred_erase, PredOp&& pred_op)
{
    size_t n_erased = 0;

    for (auto it = map.begin(); it != map.end();) {
        if (pred(*it)) {
            it = map.erase(it), ++n_erased;
        } else {
            pred_op(*it);
            ++it;
        }
    }

    return n_erased;
}

template <typename Set, typename Key>
auto find_ptr(Set&& set, Key const& key)
        -> decltype(&*set.find(key))
{
    auto it = set.find(key);
    if (it == set.end()) {
        return nullptr;
    } else {
        return &*it;
    }
}

template <typename Container, typename Elem, typename Compare = std::less<>>
auto set_push(Container&& c, Elem&& e, Compare&& compare = std::less<>{})
{
    auto iter = lower_bound(c, e, compare);
    if (iter == c.end() || compare(*iter, e) || compare(e, *iter)) {
        return c.insert(iter, std::forward<Elem>(e));
    } else {
        return (*iter = std::forward<Elem>(e)), iter;
    }
}

template <typename Set, typename Key>
auto contains(Set&& set, Key const& key)
{
    return !!find_ptr(std::forward<Set>(set), std::forward<const Key>(key));
}

template <typename Float>
auto range_alpha(Float value, Float v1, Float v2)
{
    if (v1 == v2) { return Float(value > v1); }
    auto [min, max] = std::minmax(v1, v2);

    Float alpha = std::clamp(value, min, max);
    alpha = (alpha - min) / (max - min);

    return alpha;
}

template <typename Alpha, typename ValTy>
auto lerp(ValTy const& a, ValTy const& b, Alpha const& alpha)
{
    return a + (b - a) * alpha;
}

template <typename ValTy = double, typename Iter, typename Mult = std::multiplies<>>
auto variance(Iter begin, Iter end, Mult mult = std::multiplies<>{})
{
    auto div = static_cast<ValTy>(std::distance(begin, end));
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

template <typename ValTy = double, typename Container, typename Mult = std::multiplies<>>
auto variance(Container&& cont, Mult mult = std::multiplies<>{})
{
    return variance(std::begin(cont), std::end(cont), std::forward<Mult>(mult));
}

template <class T, class Eval, class = enable_if_t<is_invocable_r_v<int, Eval, T>>>
T bsearch(T begin, T end, Eval&& eval_fn)
{
    for (;;) {
        T mid = (begin + end) / 2;
        auto eval = eval_fn(mid);

        if constexpr (std::is_integral_v<T>)
            if (begin == mid) { return mid; }

        if (eval < 0) {
            begin = mid;
        } else if (eval > 0) {
            end = mid;
        } else {
            return mid;
        }
    }
}

template <class A, class B>
int compare(A const& a, B const& b)
{
    return a < b ? -1
         : a > b ? 1
                 : 0;
}

struct owner_equal_t {
    template <typename PtrA, typename PtrB>
    bool operator()(PtrA const& a, PtrB const& b) const noexcept
    {
        return not a.owner_before(b) && not b.owner_before(a);
    }
};
constexpr owner_equal_t owner_equal{};

template <typename Any>
auto abs(Any&& val) noexcept
{
    using value_type = std::decay_t<Any>;
    return val < value_type{} ? -val : val;
}

template <class Rng, class OutputIt, class Pred, class Fct>
size_t transform_if(Rng&& rng, OutputIt dest, Pred&& pred, Fct&& transform)
{
    auto first = std::begin(rng);
    auto last = std::end(rng);
    size_t count = 0;

    while (first != last) {
        if (pred(*first)) {
            *dest++ = transform(*first);
            ++count;
        }

        ++first;
    }

    return count;
}

template <typename RandomAccessRange, class IdxRange>
auto swap_remove_index(RandomAccessRange& rng, IdxRange&& indices)
{
    auto iter_idx = std::rbegin(indices);
    auto iter_idx_end = std::rend(indices);
    auto iter_rm_begin = std::begin(rng);
    auto iter_rm_back = std::end(rng);

    assert(is_sorted(indices));
    assert(std::distance(iter_idx, iter_idx_end) <= std::distance(iter_rm_begin, iter_rm_back));

    for (; iter_idx != iter_idx_end; ++iter_idx) {
        *(iter_rm_begin + *iter_idx) = std::move(*(--iter_rm_back));
    }

    return iter_rm_back;
}

enum loop_control {
    loop_control_continue = 0,
    loop_control_break = 1,
    loop_control_remove = 2,
};

template <template <class, class> class Container, class T, class Alloc, class Fn>
void visit_swap_remove(Container<T, Alloc>& cont, Fn&& func)
{
    auto back_iter = cont.end();

    for (auto iter = cont.begin(); iter != back_iter;) {
        auto ctrl = func(*iter);

        if (ctrl & loop_control_remove) {
            *iter = move(*(--back_iter));
        } else {
            ++iter;
        }

        if (ctrl & loop_control_break) {
            break;
        }
    }

    cont.erase(back_iter, cont.end());
}

}  // namespace algorithm
}  // namespace cpph

namespace std {
inline namespace literals {
}
}  // namespace std

namespace cpph::util_ns {
using namespace std::literals;
}
