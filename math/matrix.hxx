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
#include <cmath>
#include <ostream>

#include "../array_view.hxx"
#include "../assert.hxx"
#include "defs.hxx"

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_::math {
using std::array;

template <class Lambda, int = (Lambda{}(), 0)>
constexpr bool is_constexpr(Lambda)
{
    return true;
}
constexpr bool is_constexpr(...) { return false; }

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
    constexpr static int num_rows  = Row_,
                         num_cols  = Col_,
                         length    = Row_ * Col_,
                         short_dim = Row_ < Col_ ? Row_ : Col_,
                         long_dim  = Row_ < Col_ ? Col_ : Row_;

    using value_type      = Ty_;
    using reference       = Ty_&;
    using const_reference = Ty_ const&;

    template <size_t NR_, size_t NC_>
    using matx_type = matrix<value_type, NR_, NC_>;

    template <size_t Len_>
    using vector_type = matrix<value_type, Len_, 1>;

    using row_type      = vector_type<num_cols>;
    using column_type   = vector_type<num_rows>;
    using diagonal_type = matx_type<short_dim, 1>;

   private:
    constexpr auto _cthis() const noexcept { return this; }

   public:
    /**
     * Does not initiate default-values
     */
    constexpr matrix() noexcept = default;

    /**
     * Construct matrix from row-majorly ordered values
     * @param values
     */
    explicit constexpr matrix(array_view<const value_type> values) noexcept : value()
    {
        for (int i = 0; i < length; ++i)
        {
            value[i] = values[i];
        }
    }

    explicit constexpr matrix(value_type const& v) noexcept
    {
        *this = v;
    }

    constexpr matrix(matrix const&) = default;
    constexpr matrix& operator=(matrix const&) = default;

    constexpr matrix& operator=(value_type const& v)
    {
        for (int i = 0; i < length; ++i)
            value[i] = v;

        return *this;
    }

    constexpr int rows() const noexcept { return Row_; }
    constexpr int cols() const noexcept { return Col_; }
    constexpr int size() const noexcept { return Row_ * Col_; }

   private:
    constexpr value_type* _get(int index) const noexcept
    {
        return const_cast<value_type*>(value + index);
    }

    constexpr value_type* _get(int r, int c) const noexcept
    {
        return const_cast<value_type*>(value + r * num_cols + c);
    }

   public:
    constexpr value_type const&
    operator()(int index) const noexcept
    {
        return *_get(index);
    }

    constexpr value_type&
    operator()(int index) noexcept
    {
        return *_get(index);
    }

    constexpr value_type const&
    operator()(int row, int col) const noexcept
    {
        return *_get(row, col);
    }

    constexpr value_type&
    operator()(int row, int col) noexcept
    {
        return *_get(row, col);
    }

    explicit operator value_type() const noexcept
    {
        if constexpr (length == 1)
        {
            return value[0];
        }
    }

    constexpr auto& set_diag(diagonal_type const& v) noexcept
    {
        for (int i = 0; i < short_dim; ++i)
            (*this)(i, i) = v(i);

        return *this;
    }

    auto& operator[](int index) const
    {
        assertd_(0 <= index && index < num_rows);
        return *(row_type const*)_get(index, 0);
    }

    auto& operator[](int index) noexcept
    {
        assertd_(0 <= index && index < num_rows);
        return *(row_type*)_get(index, 0);
    }

    // factory operations

    /**
     * Create matrix from arbitrary values.
     *
     * @tparam Args_
     * @param args
     */
    template <typename... Args_>
    constexpr static matrix
    create(Args_... args) noexcept
    {
        static_assert(sizeof...(args) == length);

        matrix matx = {};
        size_t n    = 0;
        ((matx(n++) = std::forward<Args_>(args)), ...);

        return matx;
    }

    /**
     * Create matrix from initializer list
     */
    constexpr static matrix
    create(std::initializer_list<value_type> values) noexcept
    {
        matrix matx = {};
        for (int i = 0; i < values.size(); ++i)
            matx(i) = values.begin()[i];

        return matx;
    }

    /**
     * Return matrix filled with single value
     *
     * @param val value to fill
     */
    constexpr static matrix
    all(value_type const& val) noexcept
    {
        matrix matx = {};
        for (int i = 0; i < length; ++i)
            matx.value[i] = val;

        return matx;
    }

    constexpr static matrix
    zeros() noexcept
    {
        return all({});
    }

    /**
     * Return matrix which is filled with given diagonal values
     *
     * @param diag diagonal values
     */
    constexpr static matrix
    diag(diagonal_type const& diag) noexcept
    {
        matrix matx = {};
        matx.set_diag(diag);
        return matx;
    }

    /**
     * Returns identity matrix
     */
    constexpr static matrix
    eye() noexcept
    {
        return diag(diagonal_type::all(1));
    }

   public:
    constexpr auto begin() const noexcept { return &*value; }
    constexpr auto end() const noexcept { return begin() + length; }

    constexpr auto cbegin() const noexcept { return &*value; }
    constexpr auto cend() const noexcept { return begin() + length; }

    constexpr auto begin() noexcept { return &*value; }
    constexpr auto end() noexcept { return begin() + length; }

   private:
    template <typename T, T... RowN_>
    constexpr auto _retr_rows(size_t n, std::integer_sequence<T, RowN_...>) const noexcept
    {
        column_type rv = {};
        ((rv.value[RowN_] = value[n + RowN_ * num_cols]), ...);
        return rv;
    }

   public:
    constexpr bool
    is_vector() const noexcept { return short_dim == 1; }

    // spanning
    constexpr auto
    row(size_t n) const noexcept
    {
        return row_type({&value[0] + n * num_cols, num_cols});
    }

    constexpr auto
    col(size_t n) const noexcept
    {
        return _retr_rows(n, std::make_index_sequence<num_rows>{});
    }

    constexpr auto
    t() const noexcept
    {
        matx_type<num_cols, num_rows> result = {};
        for (int i = 0; i < num_rows; ++i)
            for (int j = 0; j < num_cols; ++j)
                result(j, i) = (*this)(i, j);

        return result;
    }

   public:
    // vector operations
    template <int NR_, int NC_>
    constexpr value_type
    dot(matx_type<NR_, NC_> const& other) const noexcept
    {
        using other_type = matx_type<NR_, NC_>;
        static_assert(short_dim == other_type::short_dim);
        static_assert(long_dim == other_type::long_dim);

        value_type val = {};

        for (int i = 0; i < long_dim; ++i)
            val += value[i] * other.value[i];

        return val;
    }

    template <int NR_, int NC_>
    constexpr value_type
    cross(matx_type<NR_, NC_> const& other) const noexcept
    {
        using other_type = matx_type<NR_, NC_>;
        static_assert(short_dim == 1 && other_type::short_dim == 1);
        static_assert(long_dim == 3 && other_type::long_dim == 3);

        auto [a1, a2, a3] = value;
        auto [b1, b2, b3] = other.value;

        return matx_type<long_dim, short_dim>::create(
                a2 * b3 - a3 * b2, a1 * b3 - a3 * b1, a1 * b2 - a2 * b1);
    }

   private:
    template <size_t N_>
    constexpr reference _vec_at() const noexcept
    {
        static_assert(short_dim == 1);
        static_assert(long_dim >= N_);

        return const_cast<value_type&>(value[N_]);
    }

   public:  // aliases
    const_reference x() const noexcept { return _vec_at<0>(); }
    const_reference y() const noexcept { return _vec_at<1>(); }
    const_reference z() const noexcept { return _vec_at<2>(); }
    const_reference w() const noexcept { return _vec_at<3>(); }
    reference x() noexcept { return _vec_at<0>(); }
    reference y() noexcept { return _vec_at<1>(); }
    reference z() noexcept { return _vec_at<2>(); }
    reference w() noexcept { return _vec_at<3>(); }

    const_reference width() const noexcept { return _vec_at<0>(); }
    const_reference height() const noexcept { return _vec_at<1>(); }
    reference width() noexcept { return _vec_at<0>(); }
    reference height() noexcept { return _vec_at<1>(); }

    const_reference u() const noexcept { return _vec_at<0>(); }
    const_reference v() const noexcept { return _vec_at<1>(); }
    reference u() noexcept { return _vec_at<0>(); }
    reference v() noexcept { return _vec_at<1>(); }

   private:
    // arithmetic operations
    template <typename Opr_>
    constexpr matrix& _unary(Opr_&& op) noexcept
    {
        for (int i = 0; i < length; ++i)
            value[i] = op(value[i]);

        return *this;
    }

    template <typename Opr_>
    constexpr matrix& _binary(Opr_&& op, matrix const& other) noexcept
    {
        for (int i = 0; i < length; ++i)
            value[i] = op(value[i], other.value[i]);

        return *this;
    }

   public:
    constexpr matrix operator-() const noexcept { return matrix{*this}._unary(std::negate<>{}); }
    constexpr matrix operator+() const noexcept { return *this; }
    constexpr matrix operator+(matrix const& other) const noexcept { return matrix(*this) += other; }
    constexpr matrix operator-(matrix const& other) const noexcept { return matrix(*this) -= other; }

    constexpr matrix& operator+=(matrix const& other) noexcept { return _binary(std::plus<>{}, other); }
    constexpr matrix& operator-=(matrix const& other) noexcept { return _binary(std::minus<>{}, other); }

    template <int NewCol_>
    constexpr matx_type<Row_, NewCol_>
    operator*(matx_type<Col_, NewCol_> other) const noexcept
    {
        // performs matrix multiply
        matx_type<Row_, NewCol_> result = {};

        for (int i = 0; i < Row_; ++i)
            for (int j = 0; j < NewCol_; ++j)
                result(i, j) = row(i).dot(other.col(j));

        return result;
    }

    constexpr bool operator==(matrix const& other) const noexcept
    {
        for (int i = 0; i < length; ++i)
            if (value[i] != other.value[i]) { return false; }

        return true;
    }

    constexpr bool operator!=(matrix const& other) const noexcept { return not(*this == other); }

    constexpr explicit operator bool() const noexcept
    {
        for (int i = 0; i < length; ++i)
            if (not value[i]) { return false; }

        return true;
    }

    constexpr bool
    equals(matrix const& other, value_type epsilon = {}) const noexcept
    {
        for (int i = 0; i < length; ++i)
            if (std::abs(value[i] - other.value[i]) > epsilon) { return false; }

        return true;
    }

    constexpr matrix& operator/=(value_type const& val) noexcept
    {
        return _unary([&](auto&& v) { return v / val; });
    }

    constexpr matrix operator/(value_type const& val) const noexcept
    {
        return matrix{*this} /= val;
    }

    friend constexpr matrix
    operator*(value_type const& val, matrix const& mat) noexcept
    {
        return matrix{mat}._unary([&](auto&& v) { return v * val; });
    }

    friend constexpr matrix
    operator*(matrix const& mat, value_type const& val) noexcept
    {
        return val * mat;
    }

    friend std::ostream& operator<<(std::ostream& os, matrix const& matx)
    {
        os << '[';
        for (int i = 0; i < num_rows; ++i)
        {
            if (i > 0) { os << ','; }
            os << '[';
            for (int j = 0; j < num_cols; ++j)
            {
                if (j > 0) { os << ','; }
                os << matx(i, j);
            }
            os << ']';
        }
        os << ']';
        return os;
    }

   public:
    // element-wise operations
    constexpr matrix mul(matrix const& other) const noexcept
    {
        return matrix{*this}._binary(std::multiplies<>{}, other);
    }

    constexpr matrix div(matrix const& other) const noexcept
    {
        return matrix{*this}._binary(std::divides<>{}, other);
    }

   public:
    value_type value[length];
};

template <typename Ty_, int R_, int C_>
constexpr Ty_ norm_sqr(matrix<Ty_, R_, C_> const& mat) noexcept
{
    Ty_ sum = {};
    for (auto& c : mat) { sum += c * c; }

    return sum;
}

template <typename Ty_, int R_, int C_>
constexpr auto norm(matrix<Ty_, R_, C_> const& mat) noexcept
{
    return std::sqrt(norm_sqr(mat));
}

template <typename Ty_, int R_, int C_>
constexpr auto normalize(matrix<Ty_, R_, C_> const& mat) noexcept
{
    return mat / static_cast<Ty_>(norm(mat));
}

/**
 * Defines row vector
 */
template <typename Ty_, size_t N_>
using vector = matrix<Ty_, N_, 1>;

/**
 * Defines typical vectors
 */
using matx22f = matrix<float, 2, 2>;
using matx23f = matrix<float, 2, 3>;
using matx24f = matrix<float, 2, 4>;
using matx32f = matrix<float, 3, 2>;
using matx33f = matrix<float, 3, 3>;
using matx34f = matrix<float, 3, 4>;
using matx42f = matrix<float, 4, 2>;
using matx43f = matrix<float, 4, 3>;
using matx44f = matrix<float, 4, 4>;
using matx22i = matrix<int, 2, 2>;
using matx23i = matrix<int, 2, 3>;
using matx24i = matrix<int, 2, 4>;
using matx32i = matrix<int, 3, 2>;
using matx33i = matrix<int, 3, 3>;
using matx34i = matrix<int, 3, 4>;
using matx42i = matrix<int, 4, 2>;
using matx43i = matrix<int, 4, 3>;
using matx44i = matrix<int, 4, 4>;
using vec2f   = vector<float, 2>;
using vec3f   = vector<float, 3>;
using vec4f   = vector<float, 4>;
using vec2i   = vector<int, 2>;
using vec3i   = vector<int, 3>;
using vec4i   = vector<int, 4>;
using vec2b   = vector<uint8_t, 2>;
using vec3b   = vector<uint8_t, 3>;
using vec4b   = vector<uint8_t, 4>;
}  // namespace CPPHEADERS_NS_::math
