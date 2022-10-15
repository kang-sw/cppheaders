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
#include <array>
#include <cmath>
#include <ostream>

#include "../utility/array_view.hxx"
#include "defs.hxx"

//

namespace cpph::math {
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
    constexpr static int num_rows = Row_,
                         num_cols = Col_,
                         length = Row_ * Col_,
                         short_dim = Row_ < Col_ ? Row_ : Col_,
                         long_dim = Row_ < Col_ ? Col_ : Row_;

    using value_type = Ty_;
    using reference = Ty_&;
    using const_reference = Ty_ const&;

    template <int NR_, int NC_>
    using matx_type = matrix<value_type, NR_, NC_>;

    template <int Len_>
    using vector_type = matrix<value_type, Len_, 1>;

    using row_type = matx_type<1, num_cols>;
    using column_type = matx_type<num_rows, 1>;
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

    template <typename Other_>
    explicit constexpr matrix(array_view<Other_> values) noexcept : value()
    {
        for (int i = 0; i < length; ++i) {
            value[i] = values[i];
        }
    }

    constexpr matrix(value_type const* values, size_t n) noexcept : matrix(array_view{values, n}) {}

    template <typename... Args_>
    constexpr matrix(value_type const& v, Args_&&... args) noexcept : value()
    {
        static_assert(sizeof...(args) + 1 == length);
        value[0] = v;

        int n = 1;
        ((value[n++] = std::forward<Args_>(args)), ...);
    }

    constexpr matrix(matrix const&) = default;
    constexpr matrix& operator=(matrix const&) = default;

    constexpr int rows() const noexcept { return Row_; }
    constexpr int cols() const noexcept { return Col_; }
    constexpr int size() const noexcept { return Row_ * Col_; }

    template <typename Other_>
    operator matrix<Other_, Row_, Col_>() const
    {
        return matrix<Other_, Row_, Col_>{array_view{value}};
    }

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
        if constexpr (length == 1) {
            return value[0];
        }
    }

    constexpr auto& set_diag(diagonal_type const& v) noexcept
    {
        for (int i = 0; i < short_dim; ++i)
            (*this)(i, i) = v(i);

        return *this;
    }

    auto const& operator[](int index) const
    {
        return as_const(const_cast<matrix&>(*this).operator[](index));
    }

    auto& operator[](int index) noexcept
    {
        assert(0 <= index && index < ((1 == num_rows) ? num_cols : num_rows));
        if constexpr (num_cols == 1) {
            return *(value_type*)_get(index, 0);
        } else if constexpr (num_rows == 1) {
            return *(value_type*)_get(0, index);
        } else {
            return *(row_type*)_get(index, 0);
        }
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
        size_t n = 0;
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

    /**
     * Helper ... View matrix as array
     */
    auto& as_array() noexcept
    {
        if constexpr (Row_ == 1 || Col_ == 1) {
            return (array<Ty_, Row_ * Col_>&)*this;
        } else {
            return (array<array<Ty_, Col_>, Row_>&)*this;
        }
    }

    auto& as_array() const noexcept
    {
        if constexpr (Row_ == 1 || Col_ == 1) {
            return (array<Ty_, Row_ * Col_> const&)*this;
        } else {
            return (array<array<Ty_, Col_>, Row_> const&)*this;
        }
    }

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
        return row_type{&value[0] + n * num_cols, num_cols};
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

    constexpr auto
    diag() const noexcept
    {
        diagonal_type d = {};
        for (int i = 0; i < short_dim; ++i)
            d(i) = (*this)(i, i);

        return d;
    }

    bool has_nan() const noexcept
    {
        for (auto& v : value)
            if (isnan(v))
                return true;

        return false;
    }

    template <int R_, int C_, int NewR_, int NewC_>
    constexpr auto submatx() const noexcept
    {
        static_assert(0 <= R_ && R_ + NewR_ <= Row_);
        static_assert(0 <= C_ && C_ + NewC_ <= Col_);

        matx_type<NewR_, NewC_> result = {};
        for (int i = 0; i < R_; ++i)
            for (int j = 0; j < C_; ++j)
                result(i, j) = (*this)(R_ + i, C_ + j);

        return result;
    }

    template <int SubR_, int SubC_>
    constexpr matrix& update(int r, int c, matx_type<SubR_, SubC_> const& matx) noexcept
    {
        for (int i = 0; i < SubR_; ++i)
            for (int j = 0; j < SubC_; ++j)
                (*this)(r + i, c + j) = matx(i, j);

        return *this;
    }

    template <int SubR_, int SubC_>
    matrix updated(int r, int c, matx_type<SubR_, SubC_> const& matx) noexcept
    {
        return matrix{*this}.update(r, c, matx);
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
    constexpr matrix<value_type, NR_, NC_>
    cross(matx_type<NR_, NC_> const& other) const noexcept
    {
        using other_type = matx_type<NR_, NC_>;
        static_assert(short_dim == 1 && other_type::short_dim == 1);
        static_assert(long_dim == 3 && other_type::long_dim == 3);

        auto [a1, a2, a3] = value;
        auto [b1, b2, b3] = other.value;

        return matx_type<long_dim, short_dim>::create(
                a2 * b3 - a3 * b2, a3 * b1 - a1 * b3, a1 * b2 - a2 * b1);
    }

    template <int NR_, int NC_>
    constexpr matrix<value_type, NR_, NC_>
    proj(matx_type<NR_, NC_> const& other) const noexcept
    {
        using other_type = matx_type<NR_, NC_>;
        static_assert(short_dim == 1 && other_type::short_dim == 1);
        static_assert(long_dim == 3 && other_type::long_dim == 3);

        return (this->dot(other) / this->dot(*this)) * (*this);
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
    constexpr const_reference x() const noexcept { return _vec_at<0>(); }
    constexpr const_reference y() const noexcept { return _vec_at<1>(); }
    constexpr const_reference z() const noexcept { return _vec_at<2>(); }
    constexpr const_reference w() const noexcept { return _vec_at<3>(); }
    constexpr reference x() noexcept { return _vec_at<0>(); }
    constexpr reference y() noexcept { return _vec_at<1>(); }
    constexpr reference z() noexcept { return _vec_at<2>(); }
    constexpr reference w() noexcept { return _vec_at<3>(); }

    constexpr const_reference width() const noexcept { return _vec_at<0>(); }
    constexpr const_reference height() const noexcept { return _vec_at<1>(); }
    constexpr reference width() noexcept { return _vec_at<0>(); }
    constexpr reference height() noexcept { return _vec_at<1>(); }

    constexpr const_reference u() const noexcept { return _vec_at<0>(); }
    constexpr const_reference v() const noexcept { return _vec_at<1>(); }
    constexpr reference u() noexcept { return _vec_at<0>(); }
    constexpr reference v() noexcept { return _vec_at<1>(); }

    constexpr value_type area() const noexcept { return width() * height(); }

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
    constexpr matrix operator-() const noexcept
    {
        return matrix{*this}._unary([](auto&& v) { return -v; });
    }

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

    constexpr matrix&
    operator*=(value_type const& val) noexcept
    {
        return _unary([&](auto&& v) { return v * val; });
    }

    friend constexpr matrix
    operator*(value_type const& val, matrix const& mat) noexcept
    {
        return matrix{mat} *= val;
    }

    friend constexpr matrix
    operator*(matrix const& mat, value_type const& val) noexcept
    {
        return val * mat;
    }

    friend std::ostream& operator<<(std::ostream& os, matrix const& matx)
    {
        os << '[';
        for (int i = 0; i < num_rows; ++i) {
            if (i > 0) { os << ','; }
            os << '[';
            for (int j = 0; j < num_cols; ++j) {
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
        return matrix{*this}._binary([](auto&& a, auto&& b) { return a / b; }, other);
    }

   private:
    constexpr void _swap_row(int r0, int r1) noexcept
    {
        auto _0 = row(r0);
        auto _1 = row(r1);

        update(r1, 0, _0);
        update(r0, 0, _1);
    }

    constexpr void _scale_row(int r, value_type scale) noexcept
    {
        auto _0 = row(r);
        _0 *= scale;
        update(r, 0, _0);
    }

    constexpr void _scale_add(int r, row_type v, value_type scale) noexcept
    {
        auto _0 = row(r);
        _0 += v * scale;
        update(r, 0, _0);
    }

   public:
    /**
     * Invert matrix
     *
     * @param out
     * @return false if there's no solution
     */
    constexpr matrix inv() const noexcept
    {
        auto R = eye();
        auto s = *this;
        auto c = 0;

        for (; c < Col_;) {
            auto r = c;
            for (; r < Row_ && s(r, c) == 0; ++r)
                ;  // find non-zero leading coefficient

            if (r == Row_)
                continue;  // there's no non-zero leading coefficient

            if (r != c) {  // switch rows
                s._swap_row(r, c);
                R._swap_row(r, c);  // apply same on R
                r = c;              // back to current cell
            }

            {
                auto divider = Ty_(1) / s(r, c);
                s._scale_row(r, divider);
                R._scale_row(r, divider);
            }

            for (int r0 = 0; r0 < Row_; ++r0) {
                if (r0 == r) { continue; }

                auto scale = -s(r0, c);
                if (scale == 0) { continue; }

                s._scale_add(r0, s.row(r), scale);
                R._scale_add(r0, R.row(r), scale);
            }

            ++c;
        }

        return R;
    }

   public:
    value_type value[length];
};

//
// Matrix utilities
//

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

template <typename Ty_, int R_, int C_>
constexpr auto sum(matrix<Ty_, R_, C_> const& mat) noexcept
{
    Ty_ sum = {};
    for (auto& c : mat) { sum += c; }

    return sum;
}

template <typename Ty_, int R_, int C_>
constexpr auto mean(matrix<Ty_, R_, C_> const& mat) noexcept
{
    return sum(mat) / static_cast<Ty_>(R_ * C_);
}

template <typename Ty_, int R_, int C_>
constexpr auto trace(matrix<Ty_, R_, C_> const& mat) noexcept
{
    return sum(mat.diag());
}

/**
 * Defines row vector
 */
template <typename Ty_, size_t N_>
using vector = matrix<Ty_, (int)N_, 1>;

/**
 * Defines typical vectors
 */
using matx22d = matrix<double, 2, 2>;
using matx23d = matrix<double, 2, 3>;
using matx24d = matrix<double, 2, 4>;
using matx32d = matrix<double, 3, 2>;
using matx33d = matrix<double, 3, 3>;
using matx34d = matrix<double, 3, 4>;
using matx42d = matrix<double, 4, 2>;
using matx43d = matrix<double, 4, 3>;
using matx44d = matrix<double, 4, 4>;
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
using vec2d = vector<double, 2>;
using vec3d = vector<double, 3>;
using vec4d = vector<double, 4>;
using vec2f = vector<float, 2>;
using vec3f = vector<float, 3>;
using vec4f = vector<float, 4>;
using vec2i = vector<int, 2>;
using vec3i = vector<int, 3>;
using vec4i = vector<int, 4>;
using vec2s = vector<int16_t, 2>;
using vec3s = vector<int16_t, 3>;
using vec4s = vector<int16_t, 4>;
using vec2b = vector<uint8_t, 2>;
using vec3b = vector<uint8_t, 3>;
using vec4b = vector<uint8_t, 4>;
}  // namespace cpph::math
