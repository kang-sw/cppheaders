#pragma once
#include <cpph/std/optional>

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

template <typename T, size_t N_K, class = enable_if_t<std::is_floating_point_v<T>>>
auto undistort_pixel(vector<T, 2> const& P_d,
                     T const (&k)[N_K], T const (&p)[2],
                     T const (&error_thres)[2] = {1 / 3840., 1 / 2160.},  // Default 4K
                     size_t max_iteration = 10)
        -> optional<vector<T, 2>>
{
    auto& [x_d, y_d] = P_d.value;
    double err_x;
    double err_y;

    vector<T, 2> P_u_aprx = P_d;

    do {
        // Just try to distort pixel with approximated P_u
        // Calculate error between approx ~ real
        auto error = distort_pixel(P_u_aprx, k, p) - P_d;
        err_x = abs(error.x());
        err_y = abs(error.y());

        // Feedback error to source approx
        P_u_aprx -= error;
    } while (max_iteration-- && (err_x > error_thres[0] || err_y > error_thres[1]));

    if (isnan(err_x) || isnan(err_y)) {
        // f^-1(d) doesn't always have value ...?
        return {};
    }

    return P_u_aprx;
}
}  // namespace cpph::math
