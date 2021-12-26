/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2021. Seungwoo Kang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * project home: https://github.com/perfkitpp
 ******************************************************************************/

#include "math/geometry.hxx"
#include "math/matrix.hxx"
#include "math/types.hxx"
#include "third/doctest.h"

using namespace cpph::math;

TEST_SUITE("math.matrix")
{
    void foofoo()
    {
        constexpr matx33i m = matx33i::eye(), n = matx33i::create(1, 2, 3, 4, 5, 6, 7, 8, 9);
        auto c = m + (-n);

        constexpr auto cc = matx33i{}.row(2);
        static_assert(decltype(cc)::length == 3);

        constexpr int r = cc(0, 1);
        auto gk         = cc(4, 1);

        constexpr bool kk = m == n;

        matx33i g = {};
        g += c;

        g* g;
    }

    TEST_CASE("arithmetic")
    {
        auto s = matx23i::create(1, 2, 3, 4, 5, 6);

        REQUIRE(s.row(0) == matx23i::row_type::create(1, 2, 3));
        REQUIRE(s.row(1) == matx23i::row_type::create(4, 5, 6));
        REQUIRE(s.col(0) == matx23i::column_type::create(1, 4));
        REQUIRE(s.col(1) == matx23i::column_type::create(2, 5));
        REQUIRE(s.col(2) == matx23i::column_type::create(3, 6));

        REQUIRE(s == matx23i::create(1, 2, 3, 4, 5, 6));
        REQUIRE(s * 2 == matx23i::create(2, 4, 6, 8, 10, 12));
        REQUIRE(2 * s == matx23i::create(2, 4, 6, 8, 10, 12));
        REQUIRE(s / 2 == matx23i::create(0, 1, 1, 2, 2, 3));

        REQUIRE(s + s == matx23i::create(2, 4, 6, 8, 10, 12));
        REQUIRE(s - s == matx23i::zeros());
        REQUIRE(s.mul(s) == matx23i::create(1, 4, 9, 16, 25, 36));
        REQUIRE(s.div(s) == matx23i::all(1));

        REQUIRE(s.t() == matx32i::create(1, 4, 2, 5, 3, 6));
        REQUIRE(s * s.t() == matx22i::create(14, 32, 32, 77));
        REQUIRE(s.t() * s == matx33i::create(17, 22, 27, 22, 29, 36, 27, 36, 45));

        REQUIRE(norm_sqr(s) == 91);
        REQUIRE(normalize(s) == matx23i::zeros());
    }
}
