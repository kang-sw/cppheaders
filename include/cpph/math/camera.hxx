#pragma once
#include "cpph/utility/counter.hxx"
#include "geometry.hxx"
#include "matrix.hxx"

namespace cpph::math {
template <typename T, size_t N_K>
auto distort_pixel(
        vector<T, 2> const& normal,
        T const (&k)[N_K], T const (&p)[2])
        -> vector<T, 2>
{
    auto& [x_u, y_u] = normal.value;
    auto r_sq = x_u * x_u + y_u * y_u;

    T radial = 1;
    {
        auto r_sq_pow = r_sq;

        // 1 + [k[i]*r_u^2 ...]
        for (auto i : count(N_K)) {
            radial += k[i] * r_sq_pow;
            r_sq_pow *= r_sq;
        }
    }

    auto x_d = radial * x_u + 2 * p[0] * x_u * y_u + p[1] * (r_sq + 2 * x_u * x_u);
    auto y_d = radial * y_u + p[0] * (r_sq + 2 * y_u * y_u) + 2 * p[1] * x_u * y_u;

    return {x_d, y_d};
}
}  // namespace cpph::math
