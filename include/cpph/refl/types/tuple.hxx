/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2022. Seungwoo Kang
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
#include <tuple>

#include "../detail/primitives.hxx"

//
#include "../detail/object_primitive_macros.hxx"

/*
 * Tuple access
 */
namespace cpph::refl {
namespace _detail {
template <typename... Args_>
auto get_tuple_descriptor(type_tag<std::tuple<Args_...>>)
{
    static struct manip_t : templated_primitive_control<std::tuple<Args_...>> {
        entity_type type() const noexcept override
        {
            return entity_type::tuple;
        }

       protected:
        void impl_archive(archive::if_writer* strm, const std::tuple<Args_...>& data, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            strm->array_push(sizeof...(Args_));
            std::apply([strm](auto const&... args) { ((*strm << args), ...); }, data);
            strm->array_pop();
        }
        void impl_restore(archive::if_reader* strm, std::tuple<Args_...>* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            auto key = strm->begin_array();
            std::apply([strm](auto&... args) { ((*strm >> args), ...); }, *pvdata);
            strm->end_array(key);
        }
    } manip;

    return object_metadata::primitive_factory::define(sizeof(std::tuple<Args_...>), &manip);
}
}  // namespace _detail

INTERNAL_CPPH_define_(ValTy_, (is_template_instance_of<ValTy_, std::tuple>::value))
{
    static auto inst = _detail::get_tuple_descriptor(type_tag_v<ValTy_>);
    return &*inst;
}

INTERNAL_CPPH_define_(ValTy_, (is_template_instance_of<ValTy_, std::pair>::value))
{
    static struct manip_t : templated_primitive_control<ValTy_> {
        entity_type type() const noexcept override
        {
            return entity_type::tuple;
        }

        void impl_archive(archive::if_writer* strm, const ValTy_& pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            strm->array_push(2);
            *strm << pvdata.first;
            *strm << pvdata.second;
            strm->array_pop();
        }

        void impl_restore(archive::if_reader* strm, ValTy_* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
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

}  // namespace cpph::refl


