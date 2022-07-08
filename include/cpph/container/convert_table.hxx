#pragma once
#include <cpph/std/algorithm>
#include <cpph/std/list>
#include <cpph/std/vector>

#include "cpph/utility/generic.hxx"
#include "flat_map.hxx"

namespace cpph {

/**
 * Table for 1:1 matching. e.g. string to enum ...
 */
template <typename Left,
          typename Right,
          typename LCompare = std::less<Left>,
          typename RCompare = std::less<Right>>
class convert_table
{
   public:
    using left_type = Left;
    using right_type = Right;
    using value_type = pair<Left, Right>;
    using container_type = list<value_type>;
    using const_iterator = typename list<value_type>::const_iterator;

   private:
    list<value_type> _vals;
    flat_map<Left, const_iterator, LCompare> _left;
    flat_map<Right, const_iterator, RCompare> _right;

   public:
    convert_table() noexcept = default;

    convert_table(initializer_list<value_type> pairs)
    {
        assign(pairs);
    }

    template <typename LeftLike>
    Right const* find_right(LeftLike&& key) noexcept
    {
        if (auto iter = find_ptr(_left, key)) {
            return &iter->second.second;
        } else {
            return nullptr;
        }
    }

    template <typename RightLike>
    Left const* find_left(RightLike&& key) noexcept
    {
        if (auto iter = find_ptr(_right, key)) {
            return &iter->second.first;
        } else {
            return nullptr;
        }
    }

    bool insert(value_type value) noexcept
    {
        if (find_left(value.second) == nullptr && find_right(value.left) == nullptr) {
            auto iter = _vals.emplace(_vals.end(), move(value));
            _left.try_emplace(iter->first, iter);
            _right.try_emplace(iter->second, iter);
        } else {
            return false;
        }
    }

    void assign(initializer_list<value_type> pairs)
    {
        for (auto& e : pairs) {
            if (not insert(move(e)))
                throw std::logic_error{"Element must not duplicate during init!"};
        }
    }
};

}  // namespace cpph
