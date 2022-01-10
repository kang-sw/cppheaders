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

namespace CPPHEADERS_NS_::refl {
/**
 * Defines some useful expression validators
 */

#define INTERNAL_CPPH_pewpew(Name, T, Expr) \
    template <typename, class = void>       \
    constexpr bool Name = false;            \
    template <typename T>                   \
    constexpr bool Name<T, std::void_t<decltype(Expr)>> = true;

INTERNAL_CPPH_pewpew(has_reserve_v, T, std::declval<T>().reserve(0));
INTERNAL_CPPH_pewpew(has_emplace_back, T, std::declval<T>().emplace_back());
INTERNAL_CPPH_pewpew(has_emplace_front, T, std::declval<T>().emplace_front());
INTERNAL_CPPH_pewpew(has_emplace, T, std::declval<T>().emplace());

static_assert(has_reserve_v<std::vector<int>>);
static_assert(has_emplace_back<std::list<int>>);

#undef INTERNAL_CPPH_pewpew

}  // namespace CPPHEADERS_NS_::refl

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
struct primitive_manipulator : if_primitive_control
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
    static struct manip_t : if_primitive_control
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

            strm->array_push(n_elem);
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

    static struct manip_t : if_primitive_control
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

            strm->array_push(container->size());
            {
                for (auto& elem : *container)
                    *strm << elem;
            }
            strm->array_pop();
        }
        void restore(archive::if_reader* strm, void* pvdata, object_descriptor_t desc) const override
        {
            auto container = reinterpret_cast<Container_*>(pvdata);
            container->clear();

            if (not strm->is_array_next())
                throw error::invalid_read_state{}.set(strm);

            // reserve if possible
            if constexpr (has_reserve_v<Container_>)
                if (auto n = strm->num_elem_next(); n != ~size_t{})
                    container->reserve(n);

            archive::context_key key;
            strm->context(&key);

            while (not strm->should_break(key))
            {
                if constexpr (has_emplace_back<Container_>)  // maybe vector, list, deque ...
                    *strm >> container->emplace_back();
                else if constexpr (has_emplace_front<Container_>)  // maybe forward_list
                    *strm >> container->emplace_front();
                else if constexpr (has_emplace<Container_>)  // maybe set
                    *strm >> *container->emplace().first;
                else
                    Container_::ERROR_INVALID_CONTAINER;
            }
        }
        requirement_status_tag status(const void* pvdata) const noexcept override
        {
            return if_primitive_control::status(pvdata);
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
    get_object_descriptor<std::list<std::vector<int>>>();
}

}  // namespace CPPHEADERS_NS_::refl

namespace CPPHEADERS_NS_ {
/**
 * Basic chunk adapter ... simple bypasses buffer
 *
 * Adapters are basically mutable and stateful, as they are instantiated for every
 *  serialization/deserialization process
 */
struct chunk_bypass_adapter
{
    chunk_bypass_adapter(refl::object_descriptor_t) noexcept {}

    const_buffer_view
    operator()(const_buffer_view v)
    {
        return v;
    }

    mutable_buffer_view
    operator()(mutable_buffer_view v)
    {
        return v;
    }
};

// TODO: big_endian_adapter
// NOTE: for example, compress adapter, AES256 adapter, etc ...
//       As parameters can't be delievered to adapters directly,

/**
 *
 * @tparam Container_
 * @tparam Adapter_
 */
template <typename Container_,
          typename Adapter_ = chunk_bypass_adapter,
          class             = std::enable_if_t<std::is_trivial_v<typename Container_::value_type>>>
class chunk : public Container_
{
   public:
    using Container_::Container_;
    using adapter = Adapter_;

    enum
    {
        is_contiguous = false
    };
};

template <typename Container_, typename Adapter_>
class chunk<Container_,
            Adapter_,
            std::enable_if_t<
                    std::is_trivial_v<
                            remove_cvr_t<decltype(*std::data(std::declval<Container_>()))>>>>
        : public Container_
{
   public:
    using Container_::Container_;
    using adapter = Adapter_;

    enum
    {
        is_contiguous = true
    };
};

static_assert(chunk<std::vector<int>>::is_contiguous);
static_assert(not chunk<std::list<int>>::is_contiguous);

template <typename Container_, typename Adapter_>
refl::object_descriptor_ptr
initialize_object_descriptor(refl::type_tag<chunk<Container_, Adapter_>>)
{
    using chunk_t    = chunk<Container_>;
    using value_type = typename Container_::value_type;

    static struct manip_t : refl::if_primitive_control
    {
        refl::primitive_t type() const noexcept override
        {
            return refl::primitive_t::binary;
        }
        void archive(archive::if_writer* strm, const void* pvdata, refl::object_descriptor_t desc) const override
        {
            Adapter_ adapter{desc};
            auto container = (Container_ const*)pvdata;

            if constexpr (not chunk_t::is_contiguous)  // list, set, etc ...
            {
                auto total_size = sizeof(value_type) * container->size();

                strm->binary_push(total_size);
                for (auto& elem : *container)
                {
                    strm->binary_write_some(adapter(elem));
                }
                strm->binary_pop();
            }
            else
            {
                auto pdata = (chunk_t const*)pvdata;
                *strm << const_buffer_view{*pdata};
            }

            // TODO: handle endianness
        }
        void restore(archive::if_reader* strm, void* pvdata, refl::object_descriptor_t desc) const override
        {
            // TODO ...
        }
        refl::requirement_status_tag status(const void* pvdata) const noexcept override
        {
            return if_primitive_control::status(pvdata);
        }
    } manip;

    return refl::object_descriptor::primitive_factory{}
            .setup(sizeof(Container_), [] { return &manip; })
            .create();
}

inline void compile_test()
{
    refl::get_object_descriptor<chunk<std::vector<int>>>();
}

}  // namespace CPPHEADERS_NS_

namespace CPPHEADERS_NS_::refl {

}
