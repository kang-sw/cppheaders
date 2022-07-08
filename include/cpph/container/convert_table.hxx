#pragma once
#include <cpph/std/list>
#include <cpph/std/vector>

#include "cpph/utility/generic.hxx"
#include "flat_map.hxx"

namespace cpph {

/**
 * Table for 1:1 matching. e.g. string to enum ...
 */
template <typename A, typename B>
class convert_table
{
   public:
    using value_type = pair<A, B>;
    using container_type = list<value_type>;


   public:
    explicit convert_table(initializer_list<pair<A, B>> pairs);
};

}  // namespace cpph
