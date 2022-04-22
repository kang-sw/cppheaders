
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
matrix<Ty_, 3, 3> rodrigues(vector<Ty_, 3> v)
{
    using mat_t = matrix<Ty_, 3, 3>;

    auto O = norm(v);
    auto [vx, vy, vz] = (v = v / O).value;
    auto cosO = std::cos(O);
    auto sinO = std::sin(O);

    mat_t V{0, -vz, vy, vz, 0, -vx, -vy, vx, 0};
    mat_t R = cosO * mat_t::eye() + sinO * V + (Ty_(1) - cosO) * v * v.t();

    return R;
}

template <typename Ty_>
vector<Ty_, 3> rodrigues(matrix<Ty_, 3, 3> m)
{
    auto O = std::acos((trace(m) - (Ty_)1) / (Ty_)2);
    auto v = (Ty_(1) / (Ty_(2) * std::sin(O))) * vector<Ty_, 3>(m(2, 1) - m(1, 2), m(0, 2) - m(2, 0), m(1, 0) - m(0, 1));

    return v * O;
}

}  // namespace cpph::math