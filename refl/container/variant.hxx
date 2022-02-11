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
#include <variant>

#include "../detail/primitives.hxx"

//
#include "../detail/_init_macros.hxx"

namespace CPPHEADERS_NS_::refl {
namespace detail::variant {
template <typename... Args_>
using visit_fn = void (*)(archive::if_reader*, std::variant<Args_...>*);

template <typename... Args_>
using visitor_table_t = std::array<visit_fn<Args_...>, sizeof...(Args_)>;

template <size_t Int_ = 0, typename... Args_>
void assign_jump_table(visitor_table_t<Args_...>& table)
{
    if constexpr (Int_ + 1 < sizeof...(Args_)) { assign_jump_table<Int_ + 1>(table); }
    table[Int_] =
            [](archive::if_reader* strm, std::variant<Args_...>* data) {
                *strm >> std::get<Int_>(*data);
            };
}

template <typename... Args_>
void apply_variant_by_index(archive::if_reader* strm, std::variant<Args_...>* variant, size_t where)
{
    static auto table =
            [] {
                visitor_table_t<Args_...> tbl;
                assign_jump_table(tbl);
                return tbl;
            }();

    table[where](strm, variant);
}

template <typename... Args_>
auto get_metadata(type_tag<std::variant<Args_...>>)
{
    using variant_type = std::variant<Args_...>;

    static struct manip_t : templated_primitive_control<variant_type>
    {
        entity_type type() const noexcept override
        {
            return entity_type::tuple;
        }

       protected:
        void archive(archive::if_writer* strm, const variant_type& data, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            strm->array_push(2);
            *strm << data.index();
            std::visit([&](auto const& arg) { *strm << arg; }, data);
            strm->array_pop();
        }

        void restore(archive::if_reader* strm, variant_type* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
        {
            auto key   = strm->begin_array();
            auto index = ~size_t{};
            *strm >> index;

            if (index >= sizeof...(Args_))
                throw std::out_of_range("Variant out of range");

            apply_variant_by_index(strm, pvdata, index);
            strm->end_array(key);
        }

    } manip;

    static auto inst = object_metadata::primitive_factory::define(sizeof(variant_type), &manip);
    return &*inst;
}
}  // namespace detail::variant

INTERNAL_CPPH_define_(ValTy_, (is_template_instance_of<ValTy_, std::variant>::value))
{
    return detail::variant::get_metadata(type_tag_v<ValTy_>);
}
}  // namespace CPPHEADERS_NS_::refl

#include "../detail/_deinit_macros.hxx"
