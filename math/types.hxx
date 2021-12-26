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

#pragma once

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_::math {

/**
 * Rectangle descriptor. Usually manipulates values in screen position, where top left corner
 *  represents coordinate origin (0,0).
 *
 * @tparam Ty_
 */
template <typename Ty_>
class rectangle_
{
   public:
    using value_type = Ty_;
    using point_type = vector<Ty_, 2>;
    using size_type  = vector<Ty_, 2>;

   public:
    constexpr rectangle_() noexcept = default;

    constexpr rectangle_(Ty_ x_, Ty_ y_, Ty_ wid_, Ty_ hei_) noexcept
            : x(x_), y(y_), width(wid_), height(hei_) {}

    explicit constexpr rectangle_(size_type size) noexcept
            : x(0), y(0), width(size.width()), height(size.height()) {}

    constexpr auto tl() const noexcept
    {
        return point_type(x, y);
    }

    constexpr auto br() const noexcept
    {
        return point_type(x + width, y + height);
    }

    constexpr auto area() const noexcept
    {
        return width * height;
    }

    constexpr auto size() const noexcept
    {
        return size_type{width, height};
    }

    constexpr auto operator&(rectangle_ const& other) const noexcept
    {
        auto x1 = std::max(x, other.x), x2 = std::min(x + width, other.x + other.width);
        auto y1 = std::max(y, other.y), y2 = std::min(y + height, other.y + other.height);

        return from_tl_br({x1, y1}, {x2, y2});
    }

    constexpr auto operator|(rectangle_ const& other) const noexcept
    {
        auto x1 = std::min(x, other.x), x2 = std::max(x + width, other.x + other.width);
        auto y1 = std::min(y, other.y), y2 = std::max(y + height, other.y + other.height);

        return from_tl_br({x1, y1}, {x2, y2});
    }

    constexpr bool operator==(rectangle_ const& other) const noexcept
    {
        return x == other.x
            && y == other.y
            && width == other.width
            && height == other.height;
    }

    constexpr bool operator!=(rectangle_ const& other) const noexcept
    {
        return not(*this == other);
    }

    constexpr bool contains(point_type const& pt) const noexcept
    {
        return x <= pt.x() && pt.x() < x + width
            && y <= pt.y() && pt.y() < y + height;
    }

   public:
    constexpr static rectangle_
    from_tl_br(point_type tl, point_type br) noexcept
    {
        size_type size = br - tl;
        size.width()   = std::max<value_type>(0, size.width());
        size.height()  = std::max<value_type>(0, size.height());

        return rectangle_{tl.x(), tl.y(), size.width(), size.height()};
    }

    constexpr static rectangle_
    from_size(point_type xy, size_type size) noexcept
    {
        return rectangle_{xy.x(), xy.y(), size.width(), size.height()};
    }

   public:
    value_type x;
    value_type y;
    value_type width;
    value_type height;
};

using rect  = rectangle_<int>;
using rectf = rectangle_<float>;
}  // namespace CPPHEADERS_NS_::math