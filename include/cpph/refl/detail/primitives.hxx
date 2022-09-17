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
inline unique_object_metadata object_metadata::basic_factory::create()
{
    auto result = std::move(_current);
    auto& generated = *result;
    auto lookup = &generated._offset_lookup;

    assert(result->_typeid != nullptr);

    auto const n_props = generated._props.size();
    lookup->reserve(n_props);

    // for name key autogeneration
    std::vector<int> used_name_keys;

    bool const is_object = generated.is_object();
    used_name_keys.reserve(n_props);

    auto* key_table = &generated._keys;
    if (is_object)
        key_table->reserve(n_props);

#ifndef NDEBUG
    size_t object_end = 0;
#endif
    for (auto& prop : generated._props) {
        lookup->emplace_back(std::make_pair(prop.offset, prop.index_self));
        prop._owner_type = result.get();

        if (is_object) {
            if (not key_table->try_emplace(prop.name, prop.index_self).second)
                throw std::logic_error{"key must be unique!"};

            if (prop.name_key_self == 0)
                throw std::logic_error{"name key must be larger than 0!"};
            if (prop.name_key_self > 0) {
                used_name_keys.push_back(prop.name_key_self);
                push_heap(used_name_keys, std::greater<>{});
            }
        }

#ifndef NDEBUG
        object_end = std::max(object_end, prop.offset + prop.type->extent());
#endif
    }

    // assign name_key table
    if (is_object) {
        if (adjacent_find(used_name_keys) != used_name_keys.end())
            throw std::logic_error("duplicated name key assignment found!");

        // target key table
        generated._key_indices.reserve(n_props);

        size_t idx_table = 0;
        int generated_index = 1;
        for (auto& prop : generated._props) {
            // autogenerate unassigned ones
            if (prop.name_key_self < 0) {
                // skip all already pre-assigned indices
                while (not used_name_keys.empty() && used_name_keys.front() <= generated_index) {
                    generated_index = used_name_keys.front() + 1;

                    pop_heap(used_name_keys, std::greater<>{});
                    used_name_keys.pop_back();
                }

                prop.name_key_self = generated_index++;
            }

            auto const is_unique
                    = generated
                              ._key_indices
                              .try_emplace(prop.name_key_self, prop.index_self)
                              .second;

            (void)is_unique;  // prevent warning on NDEBUG
            assert(is_unique);
        }
    }

    // simply sort incrementally.
    std::sort(lookup->begin(), lookup->end());

    assert("Offset must not duplicate"
           && adjacent_find(
                      *lookup, [](auto& a, auto& b) {
                          return a.first == b.first;
                      })
                      == lookup->end());

    assert("End of address must be less or equal with actual object extent"
           && object_end <= generated.extent());

    return result;
}

}  // namespace cpph::refl

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

#include "object_primitive_macros.hxx"

namespace cpph::refl {

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
unique_object_metadata fixed_size_descriptor_impl(size_t extent, size_t num_elems)
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
            if (not strm->config.merge_on_read)
                container->clear();

            auto key = strm->begin_array();

            // reserve if possible
            if constexpr (has_reserve_v<Container_>)
                if (auto n = strm->elem_left(); n != ~size_t{})
                    container->reserve(container->size() + n);

            while (not strm->should_break(key)) {
                if constexpr (has_emplace_back<Container_>) {  // maybe vector, list, deque ...
                    *strm >> container->emplace_back();
                } else if constexpr (has_emplace_front<Container_>) {  // maybe forward_list
                    *strm >> container->emplace_front();
                } else if constexpr (has_emplace<Container_>) {  // maybe set
                    container->emplace(strm->read<value_type>());
                } else
                    Container_::ERROR_INVALID_CONTAINER;
            }

            strm->end_array(key);
        }
    } manip;

    constexpr auto extent = sizeof(Container_);
    static auto desc = object_metadata::primitive_factory::define(extent, &manip);
    return desc.get();
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
auto get_dictionary_descriptor() -> object_metadata_t
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

            if (not strm->config.merge_on_read)
                pvdata->clear();

            if constexpr (has_reserve_v<Map_>)
                if (auto elem_left = strm->elem_left(); elem_left != ~size_t{})
                    pvdata->reserve(pvdata->size() + elem_left);

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

    static auto _ = object_metadata::primitive_factory::define(sizeof(Map_), &manip);
    return _.get();
}
}  // namespace _detail

INTERNAL_CPPH_define_(MapTy_, (is_template_instance_of<MapTy_, std::map>::value))
{
    return _detail::get_dictionary_descriptor<MapTy_>();
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
    static struct manip_t : templated_primitive_control<ValTy_> {
        using value_type = remove_cvr_t<decltype(*std::declval<ValTy_>())>;
        enum { is_optional = is_template_instance_of<ValTy_, std::optional>::value };
        enum { is_unique_ptr = is_template_instance_of<ValTy_, std::unique_ptr>::value };
        enum { is_shared_ptr = is_template_instance_of<ValTy_, std::shared_ptr>::value };

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

namespace std {
inline CPPH_REFL_DEFINE_PRIM_begin(string_view)
{
    CPPH_REFL_primitive_type(string);
    CPPH_REFL_primitive_restore(_0, _1) { throw std::logic_error{"invalid restoration on view!"}; }
    CPPH_REFL_primitive_archive(strm, value) { *strm << value; }
}
CPPH_REFL_DEFINE_PRIM_end();
}  // namespace std

CPPH_REFL_DEFINE_PRIM_T_begin(T, is_template_instance_of<T, cpph::array_view>::value)
{
    CPPH_REFL_primitive_type(array);
    CPPH_REFL_primitive_restore(_0, _1) { throw std::logic_error{"invalid restoration on view!"}; }
    CPPH_REFL_primitive_archive(strm, value)
    {
        *strm << archive::push_array(value.size());
        for (auto& v : value) { *strm << v; }
        *strm << archive::pop_array;
    }
}
CPPH_REFL_DEFINE_PRIM_T_end();
