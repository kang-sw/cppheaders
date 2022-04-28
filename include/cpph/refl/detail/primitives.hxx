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

//
#pragma once
#include <optional>
#include <tuple>

#include "../../utility/cleanup.hxx"
#include "object_core.hxx"

namespace cpph::refl {
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

#undef INTERNAL_CPPH_pewpew

}  // namespace cpph::refl

#include "_init_macros.hxx"

namespace cpph::refl {

/**
 * Object descriptor
 */
template <typename ValTy_>
auto get_object_metadata_t<ValTy_, std::enable_if_t<_detail::is_cpph_refl_object_v<ValTy_>>>::operator()() const
{
    static object_metadata_ptr inst = ((ValTy_*)1)->initialize_object_metadata();

    return &*inst;
}

INTERNAL_CPPH_define_(
        ValTy_,
        (is_any_of_v<ValTy_, bool, nullptr_t, std::string>)
                || (std::is_enum_v<ValTy_> && not has_object_metadata_initializer_v<ValTy_>)
                || (std::is_integral_v<ValTy_>)
                || (std::is_floating_point_v<ValTy_>))
{
    static struct manip_t : templated_primitive_control<ValTy_> {
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

        void impl_archive(archive::if_writer* strm, const ValTy_& pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            if constexpr (std::is_enum_v<ValTy_>) {
                *strm << (std::underlying_type_t<ValTy_>)pvdata;
            } else {
                *strm << pvdata;
            }
        }

        void impl_restore(archive::if_reader* strm, ValTy_* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            if constexpr (std::is_enum_v<ValTy_>) {
                *strm >> *(std::underlying_type_t<ValTy_>*)pvdata;
            } else {
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
namespace _detail {

template <typename ElemTy_>
object_metadata_ptr fixed_size_descriptor_impl(size_t extent, size_t num_elems)
{
    static struct manip_t : templated_primitive_control<ElemTy_> {
        entity_type type() const noexcept override
        {
            return entity_type::tuple;
        }
        const object_metadata* element_type() const noexcept override
        {
            return get_object_metadata<ElemTy_>();
        }
        void impl_archive(archive::if_writer* strm,
                          ElemTy_ const& data,
                          object_metadata_t desc,
                          optional_property_metadata) const override
        {
            assert(desc->extent() % sizeof(ElemTy_) == 0);
            auto n_elem = desc->extent() / sizeof(ElemTy_);
            auto begin = (ElemTy_ const*)&data;
            auto end = begin + n_elem;

            strm->array_push(n_elem);
            std::for_each(begin, end, [&](auto&& elem) { *strm << elem; });
            strm->array_pop();
        }
        void impl_restore(archive::if_reader* strm,
                          ElemTy_* data,
                          object_metadata_t desc,
                          optional_property_metadata) const override
        {
            assert(desc->extent() % sizeof(ElemTy_) == 0);
            auto n_elem = desc->extent() / sizeof(ElemTy_);
            auto begin = data;
            auto end = begin + n_elem;

            auto context = strm->begin_array();
            std::for_each(begin, end, [&](auto&& elem) { *strm >> elem; });
            strm->end_array(context);
        }
    } manip;

    return object_metadata::primitive_factory::define(extent, &manip);
}

template <typename ElemTy, size_t NElems>
object_metadata_t fixed_size_descriptor(size_t extent)
{
    static auto desc = fixed_size_descriptor_impl<ElemTy>(extent, NElems);
    return &*desc;
}

}  // namespace _detail

INTERNAL_CPPH_define_(ValTy_, std::is_array_v<ValTy_>)
{
    return _detail::fixed_size_descriptor<
            std::remove_extent_t<ValTy_>, sizeof(ValTy_) / sizeof(std::declval<ValTy_>()[0])>(
            sizeof(ValTy_));
}

/*
 * Dynamic sized list-like containers
 *
 * (set, map, vector ...)
 */
namespace _detail {
template <typename Container_>
auto get_list_like_descriptor() -> object_metadata_t
{
    using value_type = typename Container_::value_type;

    static struct manip_t : templated_primitive_control<Container_> {
        entity_type type() const noexcept override
        {
            return entity_type::array;
        }
        object_metadata_t element_type() const noexcept override
        {
            return get_object_metadata<value_type>();
        }
        void impl_archive(archive::if_writer* strm,
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
        void impl_restore(archive::if_reader* strm,
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

            while (not strm->should_break(key)) {
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
    static auto desc = object_metadata::primitive_factory::define(extent, &manip);
    return &*desc;
}

}  // namespace _detail

INTERNAL_CPPH_define_(
        ValTy_,
        (is_template_instance_of<ValTy_, std::vector>::value))
{
    return _detail::get_list_like_descriptor<ValTy_>();
}
}  // namespace cpph::refl

/*
 * Map like access
 */
namespace cpph::refl {
namespace _detail {
template <typename Map_>
auto get_dictionary_descriptor() -> object_metadata_ptr
{
    using key_type = typename Map_::key_type;
    using mapped_type = typename Map_::mapped_type;

    static struct manip_t : templated_primitive_control<Map_> {
        entity_type type() const noexcept override
        {
            return entity_type::dictionary;
        }

        object_metadata_t element_type() const noexcept override
        {
            return get_object_metadata<std::pair<key_type, mapped_type>>();
        }

       protected:
        void impl_archive(archive::if_writer* strm, const Map_& data, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            strm->object_push(data.size());
            for (auto& [k, v] : data) {
                strm->write_key_next();
                *strm << k;
                *strm << v;
            }
            strm->object_pop();
        }

        void impl_restore(archive::if_reader* strm, Map_* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            auto ctx = strm->begin_object();
            while (not strm->should_break(ctx)) {
                std::pair<key_type, mapped_type> kv;

                strm->read_key_next();
                *strm >> kv.first;
                *strm >> kv.second;

                pvdata->emplace(std::move(kv));
            }
            strm->end_object(ctx);
        }
    } manip;

    return object_metadata::primitive_factory::define(sizeof(Map_), &manip);
}
}  // namespace _detail

INTERNAL_CPPH_define_(MapTy_, (is_template_instance_of<MapTy_, std::map>::value))
{
    static auto inst = _detail::get_dictionary_descriptor<MapTy_>();
    return &*inst;
}
}  // namespace cpph::refl

/*
 * Optionals, pointers
 */
namespace cpph::refl {
INTERNAL_CPPH_define_(
        ValTy_,
        (is_template_instance_of<ValTy_, std::optional>::value
         || is_template_instance_of<ValTy_, std::unique_ptr>::value
         || is_template_instance_of<ValTy_, std::shared_ptr>::value))
{
    using value_type = remove_cvr_t<decltype(*std::declval<ValTy_>())>;
    constexpr bool is_optional = is_template_instance_of<ValTy_, std::optional>::value;
    constexpr bool is_unique_ptr = is_template_instance_of<ValTy_, std::unique_ptr>::value;
    constexpr bool is_shared_ptr = is_template_instance_of<ValTy_, std::shared_ptr>::value;

    static struct manip_t : templated_primitive_control<ValTy_> {
        entity_type type() const noexcept override
        {
            return get_object_metadata<value_type>()->type();
        }

       protected:
        void impl_archive(archive::if_writer* strm, const ValTy_& data, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            if (not data) {
                *strm << nullptr;
                return;
            }

            *strm << *data;
        }

        void impl_restore(archive::if_reader* strm, ValTy_* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            if (not *pvdata) {
                if constexpr (is_optional)
                    (*pvdata).emplace();
                else if constexpr (is_unique_ptr)
                    (*pvdata) = std::make_unique<value_type>();
                else if constexpr (is_shared_ptr)
                    (*pvdata) = std::make_shared<value_type>();
                else
                    pvdata->INVALID_POINTER_TYPE;
            }

            *strm >> **pvdata;
        }

        requirement_status_tag impl_status(const ValTy_* data) const noexcept override
        {
            if (not data) { return requirement_status_tag::optional; }

            return (!!*data)
                         ? requirement_status_tag::optional_has_value
                         : requirement_status_tag::optional_empty;
        }
    } manip;

    static auto desc = object_metadata::primitive_factory::define(sizeof(ValTy_), &manip);
    return &*desc;
}
}  // namespace cpph::refl

#include "_deinit_macros.hxx"
