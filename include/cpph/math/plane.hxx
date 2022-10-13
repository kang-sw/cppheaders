#pragma once
#include <cmath>
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
     * Guarantees number of output_vertex does not expand
     * @return
     */
    void cull_(bool closed, array_view<vec_type const> vecs,
               void (*output_vertex)(vec_type const&),
               void (*output_fence)(size_t begin, size_t end),
               size_t index_offset) const noexcept
    {
        // Must at least be line
        if (vecs.size() < 2) { return; }

        bool is_currently_upper = calc_distance(vecs[0]) > 0;
        size_t num_out_verts = 0;
        auto idx_segment_begin = num_out_verts;

        auto fn_append_vertex =
                [&](vec_type const& vec) {
                    output_vertex(vec), ++num_out_verts;
                };

        auto fn_set_fence_here =
                [&]() {
                    // Won't allow lines on closed polygon is requested.
                    if (closed && num_out_verts - idx_segment_begin < 2) { return; }
                    output_fence(index_offset + idx_segment_begin, index_offset + num_out_verts);
                };

        auto fn_append_contact =
                [&](size_t a, size_t b) {
                    auto P_1 = vecs[a];
                    auto P_2 = vecs[b];
                    auto D = P_2 - P_1;
                    auto u = calc_u(P_1, D);

                    assert(u.has_value() && *u <= 1.001);
                    auto C = P_1 + *u * D;
                    fn_append_vertex(C);  // Insert contact point
                };

        // Checks if first upper node is incomplete. Handled lastly.
        size_t idx_first_node_contact = -1;

        // Find first contact point, if it's upper node
        if (is_currently_upper) {
            idx_first_node_contact = 0;
            auto idx_next = idx_first_node_contact + 1;  // first node contact is in upside

            for (; idx_next < vecs.size() && calc_distance(vecs[idx_next]) > 0; ++idx_next)
                ;

            // All points are in upper side
            if (idx_next == vecs.size()) {
                for (auto& v : vecs) { fn_append_vertex(v); }
                fn_set_fence_here();
                return;
            }

            // Start after first node contact
            idx_first_node_contact = idx_next - 1;
            is_currently_upper = false;
        }

        //
        for (size_t idx_now = idx_first_node_contact;;) {
            if (is_currently_upper) {
                auto idx_next = idx_now + 1;
                fn_append_vertex(vecs[idx_now]);

                // Insert series upper vertices
                for (; idx_next < vecs.size() && calc_distance(vecs[idx_next]) > 0; ++idx_next)
                    fn_append_vertex(vecs[idx_next]);

                // There are no under side vertices anymore
                if (idx_next == vecs.size()) {
                    if (idx_first_node_contact != -1) {
                        // If it's not a closed shape, cut it here once.
                        if (not closed) {
                            fn_set_fence_here();
                            idx_segment_begin = num_out_verts;
                        }

                        // Insert initial nodes ...
                        for (auto& v : vecs.subspan(0, idx_first_node_contact + 1)) {
                            fn_append_vertex(v);
                        }

                        // Insert contact of first line ...
                        fn_append_contact(idx_first_node_contact, idx_first_node_contact + 1);

                        // Finalize first shape.
                        fn_set_fence_here();
                    } else {
                        if (closed) {
                            // Means first node was in underground ...
                            fn_append_contact(vecs.size() - 1, 0);
                        }

                        // Finalize currently built one.
                        fn_set_fence_here();
                    }
                    break;
                }

                // Next node is under the plane ...
                // Find contact point and insert
                fn_append_contact(idx_next - 1, idx_next);
                fn_set_fence_here();
                assert(idx_now != 0);

                // Next node is the first under-planar one.
                idx_now = idx_next;
                is_currently_upper = false;
            } else {
                // Go forward until meet upper-planar vertex
                auto idx_next = idx_now + 1;
                for (; idx_next < vecs.size() && calc_distance(vecs[idx_next]) <= 0; ++idx_next)
                    ;

                // Iterated all nodes. There's nothing to contact with plane ...
                if (idx_next == vecs.size()) {
                    if (idx_first_node_contact != -1) {
                        idx_segment_begin = num_out_verts;

                        for (auto& v : vecs.subspan(0, idx_first_node_contact + 1)) {
                            fn_append_vertex(v);
                        }

                        fn_append_contact(idx_first_node_contact, idx_first_node_contact + 1);

                        // If it's closed shape, insert contact with under-plane node additionally.
                        if (closed) { fn_append_contact(vecs.size() - 1, 0); }

                        fn_set_fence_here();
                    }
                    break;
                }

                // Next is upper-planar vertex ... insert contact point,
                idx_segment_begin = num_out_verts;
                fn_append_contact(idx_next - 1, idx_next);

                // Next node is the first upper-planar one.
                idx_now = idx_next;
                is_currently_upper = true;
            }
        }
    }

   public:
    template <typename VertOutIt, typename IndexOutIt>
    VertOutIt cull(array_view<vec_type const> vecs,
                   VertOutIt out_vert, IndexOutIt out_idx,
                   size_t index_offset = 0,
                   bool closed = false) const noexcept
    {
        static thread_local VertOutIt* p = nullptr;
        static thread_local IndexOutIt* p2 = nullptr;
        p = &out_vert, p2 = &out_idx;

        cull_(
                closed, vecs,
                [](vec_type const& vec) { *++(*p) = vec; },
                [](size_t begin, size_t end) { *++(*p2) = {begin, end}; },
                index_offset);

        return out_vert;
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

        assert(!std::isnan(result.d_) && !std::isnan(math::norm(result.n_)));
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

template <typename T, typename V>
using std_vector_like_t = decltype(std::data(std::declval<T>()), std::size(std::declval<T>()),
                                   std::declval<T>().clear(),
                                   std::declval<T>().push_back(std::declval<V>()),
                                   (void)0);

}  // namespace cpph::math