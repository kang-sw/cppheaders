// MIT License
//
// Copyright (c) 2022. Seungwoo Kang
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

//
#pragma once
#include <array>
#include <map>
#include <optional>
#include <tuple>
#include <vector>

#include "object_impl.hxx"

namespace CPPHEADERS_NS_ {
template <typename Container_, class = std::enable_if_t<std::is_trivial_v<typename Container_::value_type>>>
class chunk : public Container_
{
   public:
    using Container_::Container_;
    enum
    {
        is_contiguous = false
    };
};

template <typename Container_>
class chunk<Container_,
            std::enable_if_t<
                    std::is_trivial_v<
                            remove_cvr_t<decltype(*std::data(std::declval<Container_>()))>>>>
        : public Container_
{
   public:
    using Container_::Container_;

    enum
    {
        is_contiguous = true
    };
};

static_assert(chunk<std::vector<int>>::is_contiguous);
static_assert(not chunk<std::list<int>>::is_contiguous);

}  // namespace CPPHEADERS_NS_

namespace CPPHEADERS_NS_::refl {

/**
 * Object descriptor
 */
template <typename ValTy_>
auto get_object_descriptor() -> object_sfinae_t<detail::is_cpph_refl_object_v<ValTy_>>
{
    static object_descriptor_ptr inst = ((ValTy_*)nullptr)->initialize_object_descriptor();

    return &*inst;
}

/*
 * Primitives
 */
namespace detail {
template <typename ValTy_>
struct primitive_manipulator : if_primitive_manipulator
{
    void archive(archive::if_writer* writer, const void* p_void, object_descriptor_t) const override
    {
        *writer << *(ValTy_*)p_void;
    }
    void restore(archive::if_reader* reader, void* p_void, object_descriptor_t) const override
    {
        *reader >> *(ValTy_*)p_void;
    }
};
}  // namespace detail

template <typename ValTy_>
auto get_object_descriptor() -> object_sfinae_t<
        (is_any_of_v<ValTy_, bool, nullptr_t, std::string>)
        || (std::is_integral_v<ValTy_>)
        || (std::is_floating_point_v<ValTy_>)  //
        >
{
    static struct manip_t : detail::primitive_manipulator<ValTy_>
    {
        primitive_t type() const noexcept override
        {
            if constexpr (std::is_integral_v<ValTy_>) { return primitive_t::integer; }
            if constexpr (std::is_floating_point_v<ValTy_>) { return primitive_t::floating_point; }
            if constexpr (std::is_same_v<ValTy_, nullptr_t>) { return primitive_t::null; }
            if constexpr (std::is_same_v<ValTy_, bool>) { return primitive_t::boolean; }
            if constexpr (std::is_same_v<ValTy_, std::string>) { return primitive_t::string; }

            return primitive_t::invalid;
        }
    } manip;

    static auto desc
            = object_descriptor::primitive_factory{}
                      .setup(sizeof(ValTy_), [] { return &manip; })
                      .create();

    return &*desc;
}

/*
 * Fixed size arrays
 */
namespace detail {
template <typename ElemTy_>
object_descriptor* fixed_size_descriptor(size_t extent, size_t num_elems)
{
    static struct manip_t : if_primitive_manipulator
    {
        primitive_t type() const noexcept override
        {
            return primitive_t::tuple;
        }
        const object_descriptor* element_type() const noexcept override
        {
            return get_object_descriptor<ElemTy_>();
        }
        void archive(archive::if_writer* strm, const void* pvdata, object_descriptor_t desc) const override
        {
            assert(desc->extent() % sizeof(ElemTy_) == 0);
            auto n_elem = desc->extent() / sizeof(ElemTy_);
            auto begin  = (ElemTy_ const*)pvdata;
            auto end    = begin + n_elem;

            strm->array_push();
            std::for_each(begin, end, [&](auto&& elem) { *strm << elem; });
            strm->array_pop();
        }
        void restore(archive::if_reader* strm, void* pvdata, object_descriptor_t desc) const override
        {
            assert(desc->extent() % sizeof(ElemTy_) == 0);
            auto n_elem = desc->extent() / sizeof(ElemTy_);
            auto begin  = (ElemTy_*)pvdata;
            auto end    = begin + n_elem;

            std::for_each(begin, end, [&](auto&& elem) { *strm >> elem; });
        }
    } manip;

    static auto desc
            = object_descriptor::primitive_factory{}
                      .setup(extent, [] { return &manip; })
                      .create();

    return &*desc;
}

template <typename>
constexpr bool is_stl_array_v = false;

template <typename Ty_, size_t N_>
constexpr bool is_stl_array_v<std::array<Ty_, N_>> = true;

}  // namespace detail

template <typename ValTy_>
auto get_object_descriptor() -> object_sfinae_t<std::is_array_v<ValTy_>>
{
    return detail::fixed_size_descriptor<std::remove_extent_t<ValTy_>>(
            sizeof(ValTy_),
            std::size(*(ValTy_*)0));
}

template <typename ValTy_>
auto get_object_descriptor() -> object_sfinae_t<detail::is_stl_array_v<ValTy_>>
{
    return detail::fixed_size_descriptor<typename ValTy_::value_type>(
            sizeof(ValTy_),
            std::size(*(ValTy_*)0));
}

/*
 * Dynamic sized list-like containers
 *
 * (set, map, vector ...)
 */
namespace detail {
template <typename Container_>
auto get_list_like_descriptor() -> object_descriptor_t
{
    using value_type = typename Container_::value_type;

    static struct manip_t : if_primitive_manipulator
    {
        primitive_t type() const noexcept override
        {
            return primitive_t::array;
        }
        object_descriptor_t element_type() const noexcept override
        {
            return get_object_descriptor<value_type>();
        }
        void archive(archive::if_writer* strm, const void* pvdata, object_descriptor_t desc) const override
        {
            auto container = reinterpret_cast<Container_ const*>(pvdata);

            for (auto& elem : *container)
                *strm << elem;
        }
        void restore(archive::if_reader* strm, void* pvdata, object_descriptor_t desc) const override
        {

        }
        requirement_status_tag status(const void* pvdata) const noexcept override
        {
            return if_primitive_manipulator::status(pvdata);
        }
    } manip;

    constexpr auto extent = sizeof(Container_);
    static auto desc
            = object_descriptor::primitive_factory{}
                      .setup(extent, [] { return &manip; })
                      .create();

    return &*desc;
}
}  // namespace detail

template <typename ValTy_>
auto get_object_descriptor()
        -> object_sfinae_t<
                is_template_instance_of<ValTy_, std::vector>::value
                || is_template_instance_of<ValTy_, std::list>::value>
{
    return detail::get_list_like_descriptor<ValTy_>();
}

inline void compile_test()
{
    get_object_descriptor<std::vector<int>>();
}

}  // namespace CPPHEADERS_NS_::refl
