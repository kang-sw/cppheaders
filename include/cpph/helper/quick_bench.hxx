#pragma once
#include <cstdio>
#include <stdexcept>
#include <string_view>

#include "../algorithm.hxx"
#include "../array_view.hxx"
#include "../utility/chrono.hxx"

namespace cpph::helper {
template <int MaxEntity>
class quick_bench
{
    using clock_type = std::chrono::high_resolution_clock;
    static_assert(MaxEntity > 0);

    char _keys[MaxEntity][64];
    clock_type::duration _elapse[MaxEntity];
    clock_type::time_point _pivot = clock_type::now();
    int _key_cursor = 0;
    int _cursor = 0;

   public:
    void clear() noexcept { _cursor = 0; }
    void reset() noexcept { _pivot = clock_type::now(); }
    void step(std::string_view key)
    {
        auto delta = clock_type::now() - _pivot;
        if (_cursor + 1 == MaxEntity) { throw std::logic_error{"Too many elapses!"}; }

        auto cursor = _cursor++;

        key = key.substr(0, sizeof(_keys[0]) - 1);
        copy(key, _keys[cursor]);
        _keys[cursor][key.size()] = 0;
        _elapse[cursor] = delta;

        _pivot = clock_type::now();
    }

    template <typename Visitor>
    void print(Visitor&& visit) const
    {
        for (auto i = 0; i < _cursor; ++i) {
            visit(std::string_view(_keys[i]), _elapse[i]);
        }
    }
};
}  // namespace cpph::helper
