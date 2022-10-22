
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
#include "matrix.hxx"

//

namespace cpph::math {
template <typename Ty_>
matrix<Ty_, 3, 3> rodrigues(vector<Ty_, 3> v, Ty_ epsilon = 1e-9)
{
    using mat_t = matrix<Ty_, 3, 3>;

    auto O = norm(v);
    if (O <= epsilon) { return matrix<Ty_, 3, 3>::eye(); }
    auto [vx, vy, vz] = (v = v / O).value;
    auto cosO = std::cos(O);
    auto sinO = std::sin(O);

    mat_t V{0, -vz, vy, vz, 0, -vx, -vy, vx, 0};
    mat_t R = cosO * mat_t::eye() + sinO * V + (Ty_(1) - cosO) * v * v.t();

    return R;
}

template <typename Ty_>
vector<Ty_, 3> rodrigues(matrix<Ty_, 3, 3> m, Ty_ epsilon = 1e-9)
{
    auto O = std::acos((trace(m) - (Ty_)1) / (Ty_)2);
    if (O <= epsilon) { return {}; }
    auto v = (Ty_(1) / (Ty_(2) * std::sin(O))) * vector<Ty_, 3>(m(2, 1) - m(1, 2), m(0, 2) - m(2, 0), m(1, 0) - m(0, 1));

    return v * O;
}

enum class coord {
    zero,
    x = 1,
    y = 2,
    z = 3,
    _mx = -x,
    _my = -y,
    _mz = -z,
};

inline coord operator-(coord v) noexcept { return coord(-(int)v); }

template <typename T>
auto convert_coord(matrix<T, 3, 3> rmat, coord src_x, coord src_y, coord src_z)
{
    using matrix_type = matrix<T, 3, 3>;
    using row_type = typename matrix_type::row_type;
    matrix_type rslt = matrix_type::eye();

    // right X, down Y, forward Z  to  right X, forward Y, up Z

    row_type src[] = {rmat.row(0), rmat.row(1), rmat.row(2)};
    coord coords[] = {src_x, src_y, src_z};

    for (int idx_dst = 0; idx_dst < 3; ++idx_dst) {
        int idx_src = (int)coords[idx_dst];
        row_type row_src;

        if (idx_src > 0) {
            row_src = src[idx_src - 1];
        } else {
            row_src = -src[-idx_src - 1];
        }

        rslt.update(idx_dst, 0, row_src);
    }

    return rslt;
}

template <typename T, size_t R>
bool find_nearest(
        math::vector<T, R> const& P1, math::vector<T, R> const& D1,
        math::vector<T, R> const& P2, math::vector<T, R> const& D2,
        out<T> alpha_1, out<T> alpha_2,
        T epsilon = 1e-6)
{
    auto D1xD2 = D1.cross(D2);
    auto cross_sqr = norm_sqr(D1xD2);

    if (abs(cross_sqr) < epsilon) { return false; }
    auto s = (P2 - P1).cross(D2).dot(D1xD2) / cross_sqr;
    auto t = (P1 - P2).cross(D1).dot(-D1xD2) / cross_sqr;

    *alpha_1 = s;
    *alpha_2 = t;
    return true;
}

template <typename T, size_t R>
double calc_distance_sqr(
        math::vector<T, R> const& line_p,
        math::vector<T, R> const& line_dir,
        math::vector<T, R> const& point,
        math::vector<T, R>* out_contact_pt = nullptr)
{
    auto dot_dir = (point - line_p);
    auto contact = line_dir.proj(dot_dir);

    if (out_contact_pt) { *out_contact_pt = line_p + contact; }
    return norm_sqr(dot_dir - contact);
}
}  // namespace cpph::math