#pragma once
#include <cpph/std/array>
#include <cpph/std/optional>

#include "matrix.hxx"

//

namespace cpph::math {
template <typename T>
class plane
{
    using vec_type = vector<T, 3>;
    using underlying_type = T;

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
        auto base = n_.dot(D);
        auto value = n_.dot(P_3 - P_1);

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
    size_t cull_(bool closed, array_view<vec_type const> vecs, void (*output)(vec_type const&)) const noexcept
    {
        // Must at least be line
        if (vecs.size() < 2) { return 0; }

        size_t num_out_points = 0;
        bool is_currently_upper = calc_distance(vecs[0]) > 0;
        bool const first_node_upper = is_currently_upper;

        auto fn_insert_contact =
                [&](size_t a, size_t b) {
                    auto P_1 = vecs[a];
                    auto D = vecs[b] - P_1;
                    auto u = calc_u(P_1, D);

                    assert(u.has_value());
                    auto C = P_1 + *u * D;
                    output(C), ++num_out_points;  // Insert contact point
                };

        //
        for (size_t idx_now = 0;;) {
            if (is_currently_upper) {
                output(vecs[idx_now]), ++num_out_points;
                auto idx_next = idx_now + 1;

                // Insert series upper vertices
                for (; idx_next < vecs.size() && calc_distance(idx_next) > 0; ++idx_next)
                    output(vecs[idx_next]), ++num_out_points;

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

        return num_out_points;
    }

   public:
    template <typename OutIt>
    size_t cull(array_view<vec_type const> vecs, OutIt out, bool closed = false) const noexcept
    {
        static thread_local OutIt* p = nullptr;
        p = &out;

        return cull_(closed, vecs, [](vec_type const& vec) { *++(*p) = vec; });
    }

   public:
    //! Normal is calculated by right-hand way
    static plane from_triangle(array<vec_type, 3> const& vecs)
    {
        // TODO
        return {};
    }
};

}  // namespace cpph::math