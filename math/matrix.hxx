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
#include <array>

#include "../array_view.hxx"
#include "../assert.hxx"

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_::math {
/**
 * Defines matrix
 *
 * @tparam Ty_ Value type
 * @tparam Row_ Number of rows
 * @tparam Col_ Number of cols
 */
template <typename Ty_, int Row_, int Col_>
class matrix
{
   public:
    using value_type = Ty_;

    enum
    {
        num_rows = Row_,
        num_cols = Col_,
        length   = Row_ * Col_
    };

   public:
    /**
     * Construct matrix from row-majorly ordered values
     * @param values
     */
    matrix(array_view<Ty_> values) noexcept
    {
        assertd_(values.size() == Row_ * Col_);
    }

    matrix(matrix const&) = default;
    matrix& operator=(matrix const&) = default;

    constexpr int rows() const noexcept { return Row_; }
    constexpr int cols() const noexcept { return Col_; }
    constexpr int size() const noexcept { return Row_ * Col_; }

    auto data() const noexcept { return static_cast<value_type const*>(_value); }
    auto data() noexcept { return static_cast<value_type*>(_value); }

    auto& array() const noexcept { return (std::array<value_type, length> const&)_value; }
    auto& array() noexcept { return (std::array<value_type, length>&)_value; }

    auto& operator()(int index) const noexcept { return array()[index]; }
    auto& operator()(int index) noexcept { return array()[index]; }

    auto& operator()(int row, int col) const noexcept
    {
        assert_(0 <= row && row < num_rows);
        assert_(0 <= col && col < num_cols);

        return _value[row][col];
    }

    auto& operator()(int r, int c) noexcept { return (value_type&)_const_self()(r, c); }

    // factory operations


    // arithmetic operations


   private:
    matrix const& _const_self() const noexcept { return *this; }

   private:
    value_type _value[num_rows][num_cols];
};

/**
 * Defines row vector
 */
template <typename Ty_, size_t N_>
using vector = matrix<Ty_, N_, 1>;

/**
 * Defines typical vectors
 */
 

}  // namespace CPPHEADERS_NS_::math