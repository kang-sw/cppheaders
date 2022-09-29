#pragma once
#include <cpph/std/array>
#include <cpph/std/optional>

#include "matrix.hxx"

//

namespace cpph::math {
template <typename T, size_t Dim = 3>
class plane
{
    using vec_type = vector<T, Dim>;
    using underlying_type = T;
    enum { dimension = Dim };

   private:
    vec_type n_;  // Normal
    T d_;         // Distance

   public:
    plane() noexcept = default;
    plane(vec_type const& n, T d) noexcept : n_(normalize(n)), d_(d) {}

   public:
    auto const& n() const noexcept { return n_; }
    auto const& d() const noexcept { return d; }
    void n(vec_type const& value) noexcept { n_ = normalize(value); }
    void d(double value) noexcept { d_ = value; }

   public:
    // P_1 (u=0) -------> P_1 + D (u=1)
    auto calc_u(vec_type const& P_1, vec_type const& D) const noexcept -> optional<double>
    {
        auto const P_3 = n_ * d_;
        auto value = n_.dot(P_3 - P_1);
        auto base = n_.dot(D);

        if (std::abs(base) < 1e-7) { return {}; }
        return value / base;
    }

    auto calc_distance(vec_type const& P) const noexcept -> double
    {
        auto const P_3 = n_ * d_;

        // Project P3 onto normal
        return (P - P_3).dot(n_);
    }

   private:
    /**
     *
     * @param vecs Vector must be in closed form! (e.g. vecs[0] == vecs[vecs.size()-1]
     * @return
     */
    void cull_(bool closed, array_view<vec_type const> vecs, void (*output)(vec_type const&)) const noexcept
    {
        // Must at least be line
        if (vecs.size() < 2) { return; }

        bool is_currently_upper = calc_distance(vecs[0]) > 0;
        bool const first_node_upper = is_currently_upper;

        auto fn_insert_contact =
                [&](size_t a, size_t b) {
                    auto P_1 = vecs[a];
                    auto P_2 = vecs[b];
                    auto D = P_2 - P_1;
                    auto u = calc_u(P_1, D);

                    assert(u.has_value() && *u <= 1.001);
                    auto C = P_1 + *u * D;
                    output(C);  // Insert contact point
                };

        //
        for (size_t idx_now = 0;;) {
            if (is_currently_upper) {
                output(vecs[idx_now]);
                auto idx_next = idx_now + 1;

                // Insert series upper vertices
                for (; idx_next < vecs.size() && calc_distance(vecs[idx_next]) > 0; ++idx_next)
                    output(vecs[idx_next]);

                // There are no under side vertices anymore
                if (idx_next == vecs.size())
                    break;

                // Next node is under the plane ...
                // Find contact point and insert
                fn_insert_contact(idx_next - 1, idx_next);

                // Next node is the first under-planar one.
                idx_now = idx_next;
                is_currently_upper = false;
            } else {
                // Go forward until meet upper-planar vertex
                auto idx_next = idx_now + 1;
                for (; idx_next < vecs.size() && calc_distance(vecs[idx_next]) <= 0; ++idx_next)
                    ;

                // Iterated all nodes. There's nothing to contact with plane ...
                if (idx_next == vecs.size())
                    break;

                // Next is upper-planar vertex ... insert contact point,
                fn_insert_contact(idx_next - 1, idx_next);

                // Next node is the first upper-planar one.
                idx_now = idx_next;
                is_currently_upper = true;
            }
        }

        // On last node, if profile mismatches, insert contact point ...
        if (closed && is_currently_upper != first_node_upper) {
            fn_insert_contact(0, vecs.size() - 1);
        }
    }

   public:
    template <typename OutIt>
    OutIt cull(array_view<vec_type const> vecs, OutIt out, bool closed = false) const noexcept
    {
        static thread_local OutIt* p = nullptr;
        p = &out;

        cull_(closed, vecs, [](vec_type const& vec) { *++(*p) = vec; });
        return out;
    }

   public:
    //! Normal is calculated by right-hand way
    static optional<plane> from_triangle(array<vec_type, 3> const& p) noexcept
    {
        auto& [p1, p2, p3] = p;
        vec_type A = p2 - p1;
        vec_type B = p3 - p1;

        vec_type D = (A.cross(B));
        auto norm = math::norm(D);
        if (norm == 0) { return {}; }

        plane<double> result = {};
        result.n_ = D / norm;
        result.d_ = result.n_.dot(p1);

        assert(!isnan(result.d_) && !isnan(math::norm(result.n_)));
        return result;
    }
};

//        {
//            static vector<math::vec3d> temporary_vectors[2];
//            auto& [pts_frustum, tmp] = temporary_vectors;
//            pts_frustum.clear();
//
//            // Iterate each frustum
//            for (auto& frustum : f.frustum) {
//                tmp.clear();
//
//                frustum.cull(dots_view, back_inserter(tmp), closed);
//
//                swap(tmp, pts_frustum);
//
//                // Reset view to result dots ...
//                dots_view = pts_frustum;
//            }
//        }

template <
        class T, size_t Dim,
        class VectorLike,
        class = decltype(array_view{std::declval<VectorLike>()},
                         std::declval<VectorLike>().clear(),
                         std::declval<VectorLike>().push_back(std::declval<vector<T, Dim>>()))>
void cull_frustum(
        const_array_view<plane<T, Dim>> frustum,
        VectorLike& dots, VectorLike& out, bool is_closed = false)
{
    assert(&out != &dots);
    const_array_view<vector<T, Dim>> dots_view{dots};

    for (auto& pl : frustum) {
        out.clear();
        pl.cull(dots_view, back_inserter(out), is_closed);

        swap(dots, out);
        dots_view = dots;
    }

    if (dots_view.data() != out.data()) {
        swap(out, dots);
        assert(out.data() == dots_view.data());
    }
}
}  // namespace cpph::math