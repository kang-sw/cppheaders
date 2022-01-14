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

#include "../utility/cleanup.hxx"
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
INTERNAL_CPPH_pewpew(has_resize, T, std::declval<T>().resize(0));
INTERNAL_CPPH_pewpew(has_data, T, std::data(std::declval<T>()));

static_assert(has_reserve_v<std::vector<int>>);
static_assert(has_emplace_back<std::list<int>>);

#undef INTERNAL_CPPH_pewpew

}  // namespace CPPHEADERS_NS_::refl

namespace CPPHEADERS_NS_::refl {

/**
 * Object descriptor
 */
template <typename ValTy_>
auto get_object_metadata() -> object_sfinae_t<detail::is_cpph_refl_object_v<ValTy_>>
{
    static object_metadata_ptr inst = ((ValTy_*)nullptr)->initialize_object_metadata();

    return &*inst;
}

template <typename ValTy_>
auto get_object_metadata() -> object_sfinae_t<
        (is_any_of_v<ValTy_, bool, nullptr_t, std::string>)
        || (std::is_enum_v<ValTy_> && not has_object_metadata_initializer_v<ValTy_>)
        || (std::is_integral_v<ValTy_>)
        || (std::is_floating_point_v<ValTy_>)  //
        >
{
    static struct manip_t : templated_primitive_control<ValTy_>
    {
        entity_type type() const noexcept override
        {
            if constexpr (std::is_enum_v<ValTy_>) { return entity_type::integer; }
            if constexpr (std::is_integral_v<ValTy_>) { return entity_type::integer; }
            if constexpr (std::is_floating_point_v<ValTy_>) { return entity_type::floating_point; }
            if constexpr (std::is_same_v<ValTy_, nullptr_t>) { return entity_type::null; }
            if constexpr (std::is_same_v<ValTy_, bool>) { return entity_type::boolean; }
            if constexpr (std::is_same_v<ValTy_, std::string>) { return entity_type::string; }

            return entity_type::invalid;
        }

        void archive(archive::if_writer* strm, const ValTy_& pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            if constexpr (std::is_enum_v<ValTy_>)
            {
                *strm << (std::underlying_type_t<ValTy_>)pvdata;
            }
            else
            {
                *strm << pvdata;
            }
        }

        void restore(archive::if_reader* strm, ValTy_* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            if constexpr (std::is_enum_v<ValTy_>)
            {
                *strm >> *(std::underlying_type_t<ValTy_>*)pvdata;
            }
            else
            {
                *strm >> *pvdata;
            }
        }
    } manip;

    static auto desc = object_metadata::primitive_factory::define(sizeof(ValTy_), &manip);
    return &*desc;
}

/*
 * Fixed size arrays
 */
namespace detail {
template <typename ElemTy_>
object_metadata_t fixed_size_descriptor(size_t extent, size_t num_elems)
{
    static struct manip_t : templated_primitive_control<ElemTy_>
    {
        entity_type type() const noexcept override
        {
            return entity_type::tuple;
        }
        const object_metadata* element_type() const noexcept override
        {
            return get_object_metadata<ElemTy_>();
        }
        void archive(archive::if_writer* strm,
                     ElemTy_ const& data,
                     object_metadata_t desc,
                     optional_property_metadata) const override
        {
            assert(desc->extent() % sizeof(ElemTy_) == 0);
            auto n_elem = desc->extent() / sizeof(ElemTy_);
            auto begin  = (ElemTy_ const*)&data;
            auto end    = begin + n_elem;

            strm->array_push(n_elem);
            std::for_each(begin, end, [&](auto&& elem) { *strm << elem; });
            strm->array_pop();
        }
        void restore(archive::if_reader* strm,
                     ElemTy_* data,
                     object_metadata_t desc,
                     optional_property_metadata) const override
        {
            assert(desc->extent() % sizeof(ElemTy_) == 0);
            auto n_elem = desc->extent() / sizeof(ElemTy_);
            auto begin  = data;
            auto end    = begin + n_elem;

            auto context = strm->begin_array();
            std::for_each(begin, end, [&](auto&& elem) { *strm >> elem; });
            strm->end_array(context);
        }
    } manip;

    static auto desc = object_metadata::primitive_factory::define(extent, &manip);
    return &*desc;
}

template <typename>
constexpr bool is_stl_array_v = false;

template <typename Ty_, size_t N_>
constexpr bool is_stl_array_v<std::array<Ty_, N_>> = true;

}  // namespace detail

template <typename ValTy_>
auto get_object_metadata() -> object_sfinae_t<std::is_array_v<ValTy_>>
{
    return detail::fixed_size_descriptor<std::remove_extent_t<ValTy_>>(
            sizeof(ValTy_),
            std::size(*(ValTy_*)0));
}

template <typename ValTy_>
auto get_object_metadata() -> object_sfinae_t<detail::is_stl_array_v<ValTy_>>
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
auto get_list_like_descriptor() -> object_metadata_t
{
    using value_type = typename Container_::value_type;

    static struct manip_t : templated_primitive_control<Container_>
    {
        entity_type type() const noexcept override
        {
            return entity_type::array;
        }
        object_metadata_t element_type() const noexcept override
        {
            return get_object_metadata<value_type>();
        }
        void archive(archive::if_writer* strm,
                     const Container_& data,
                     object_metadata_t desc,
                     optional_property_metadata) const override
        {
            auto container = &data;

            strm->array_push(container->size());
            {
                for (auto& elem : *container)
                    *strm << elem;
            }
            strm->array_pop();
        }
        void restore(archive::if_reader* strm,
                     Container_* container,
                     object_metadata_t desc,
                     optional_property_metadata) const override
        {
            container->clear();
            auto key = strm->begin_array();

            // reserve if possible
            if constexpr (has_reserve_v<Container_>)
                if (auto n = strm->elem_left(); n != ~size_t{})
                    container->reserve(n);

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

            strm->end_array(key);
        }
    } manip;

    constexpr auto extent = sizeof(Container_);
    static auto desc      = object_metadata::primitive_factory::define(extent, &manip);
    return &*desc;
}
}  // namespace detail

template <typename ValTy_>
auto get_object_metadata()
        -> object_sfinae_t<
                is_template_instance_of<ValTy_, std::vector>::value
                || is_template_instance_of<ValTy_, std::list>::value>
{
    return detail::get_list_like_descriptor<ValTy_>();
}

}  // namespace CPPHEADERS_NS_::refl

/*
 * Map like access
 */
namespace CPPHEADERS_NS_::refl {
namespace detail {
template <typename Map_>
auto get_dictionary_descriptor() -> object_metadata_ptr
{
    // TODO
}
}  // namespace detail

template <typename MapTy_>
auto get_object_metadata()
        -> object_sfinae_t<is_template_instance_of<MapTy_, std::map>::value>
{
    static auto inst = detail::get_dictionary_descriptor<MapTy_>();
    return &*inst;
}
}  // namespace CPPHEADERS_NS_::refl

/*
 * Tuple access
 */
namespace CPPHEADERS_NS_::refl {
namespace detail {
template <typename... Args_>
auto get_tuple_descriptor(type_tag<std::tuple<Args_...>>)
{
    static struct manip_t : templated_primitive_control<std::tuple<Args_...>>
    {
        entity_type type() const noexcept override
        {
            return entity_type::integer;
        }

       protected:
        void archive(archive::if_writer* strm, const std::tuple<Args_...>& data, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            strm->array_push(sizeof...(Args_));
            std::apply([strm](auto const&... args) { ((*strm << args), ...); }, data);
            strm->array_pop();
        }
        void restore(archive::if_reader* strm, std::tuple<Args_...>* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            auto key = strm->begin_array();
            std::apply([strm](auto&... args) { ((*strm >> args), ...); }, *pvdata);
            strm->end_array(key);
        }
    } manip;

    return object_metadata::primitive_factory::define(sizeof(std::tuple<Args_...>), &manip);
}
}  // namespace detail

template <typename ValTy_>
auto get_object_metadata()
        -> object_sfinae_t<is_template_instance_of<ValTy_, std::tuple>::value>
{
    static auto inst = detail::get_tuple_descriptor(type_tag_v<ValTy_>);
    return &*inst;
}

template <typename ValTy_>
auto get_object_metadata()
        -> object_sfinae_t<is_template_instance_of<ValTy_, std::pair>::value>
{
    static struct manip_t : templated_primitive_control<ValTy_>
    {
        entity_type type() const noexcept override
        {
            return entity_type::tuple;
        }
        void archive(archive::if_writer* strm, const ValTy_& pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            strm->array_push(2);
            *strm << pvdata.first;
            *strm << pvdata.second;
            strm->array_pop();
        }
        void restore(archive::if_reader* strm, ValTy_* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            auto key = strm->begin_array();
            *strm >> pvdata->first;
            *strm >> pvdata->second;
            strm->end_array(key);
        }
    } manip;

    static auto desc = object_metadata::primitive_factory::define(sizeof(ValTy_), &manip);
    return &*desc;
}

}  // namespace CPPHEADERS_NS_::refl

/*
 * TODO: Optional
 */

/*
 * TODO: Variant
 */

/*
 * Binary
 */
namespace CPPHEADERS_NS_ {
template <typename Container_, class = void>
class binary;

template <typename Container_>
class binary<
        Container_,
        std::enable_if_t<
                (is_binary_compatible_v<typename Container_::value_type>)  //
                &&(not refl::has_data<Container_>)>>
        : public Container_
{
   public:
    using Container_::Container_;

    enum
    {
        is_container  = true,
        is_contiguous = false
    };
};

template <typename Container_>
class binary<Container_,
             std::enable_if_t<
                     is_binary_compatible_v<
                             remove_cvr_t<decltype(*std::data(std::declval<Container_>()))>>>>
        : public Container_
{
   public:
    using Container_::Container_;

    enum
    {
        is_container  = true,
        is_contiguous = true
    };
};

template <typename ValTy_>
class binary<ValTy_, std::enable_if_t<is_binary_compatible_v<ValTy_>>> : ValTy_
{
   public:
    using ValTy_::ValTy_;

    enum
    {
        is_container = false,
    };
};

static_assert(binary<std::vector<int>>::is_contiguous);
static_assert(not binary<std::list<int>>::is_contiguous);

template <typename Container_>
refl::object_metadata_ptr
initialize_object_metadata(refl::type_tag<binary<Container_>>)
{
    using binary_type = binary<Container_>;

    static struct manip_t : refl::templated_primitive_control<binary_type>
    {
        refl::entity_type type() const noexcept override
        {
            return refl::entity_type::binary;
        }
        void archive(archive::if_writer* strm,
                     const binary_type& pvdata,
                     refl::object_metadata_t desc,
                     refl::optional_property_metadata prop) const override
        {
            auto data = &pvdata;

            if constexpr (not binary_type::is_container)
            {
                *strm << const_buffer_view{data, 1};
            }
            else if constexpr (binary_type::is_contiguous)  // list, set, etc ...
            {
                *strm << const_buffer_view{*data};
            }
            else
            {
                using value_type = typename Container_::value_type;
                auto total_size  = sizeof(value_type) * std::size(*data);

                strm->binary_push(total_size);
                for (auto& elem : *data)
                {
                    strm->binary_write_some(const_buffer_view{&elem, 1});
                }
                strm->binary_pop();
            }
        }
        void restore(archive::if_reader* strm,
                     binary_type* data,
                     refl::object_metadata_t desc,
                     refl::optional_property_metadata prop) const override
        {
            auto binsize                = strm->begin_binary();
            [[maybe_unused]] auto clean = cleanup([&] { strm->end_binary(); });

            if constexpr (not binary_type::is_container)
            {
                strm->binary_read_some(mutable_buffer_view{data, 1});
            }
            else
            {
                using value_type = typename Container_::value_type;
                auto elemsize    = binsize / sizeof(value_type);

                if (binsize % sizeof(value_type) != 0)
                    throw refl::error::primitive{}.set(strm).message("Byte data alignment mismatch");

                if constexpr (binary_type::is_contiguous)
                {
                    if constexpr (refl::has_resize<Container_>)
                    {
                        // If it's dynamic, read all.
                        data->resize(elemsize);
                    }

                    strm->binary_read_some({std::data(*data), std::size(*data)});
                }
                else
                {
                    if constexpr (refl::has_reserve_v<Container_>)
                    {
                        data->reserve(elemsize);
                    }

                    value_type* mutable_data = {};
                    for (auto idx : perfkit::count(elemsize))
                    {
                        if constexpr (refl::has_emplace_back<Container_>)  // maybe vector, list, deque ...
                            mutable_data = &data->emplace_back();
                        else if constexpr (refl::has_emplace_front<Container_>)  // maybe forward_list
                            mutable_data = &data->emplace_front();
                        else if constexpr (refl::has_emplace<Container_>)  // maybe set
                            mutable_data = &*data->emplace().first;
                        else
                            Container_::ERROR_INVALID_CONTAINER;

                        strm->binary_read_some({mutable_data, 1});
                    }
                }
            }
        }
    } manip;

    return refl::object_metadata::primitive_factory::define(sizeof(Container_), &manip);
}

/*
 * TODO: Trivial binary with contiguous memory chunk, which can apply read/write adapters
 */

}  // namespace CPPHEADERS_NS_
