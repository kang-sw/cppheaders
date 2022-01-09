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
#include <map>
#include <optional>
#include <set>
#include <variant>

#include "object_impl.hxx"

//
#include "../../__namespace__.h"
namespace CPPHEADERS_NS_::refl {

template <typename ValTy_>
struct primitive_manipulator : if_primitive_manipulator
{
    void archive(archive::if_writer* writer, const void* p_void) const override
    {
        *writer << *(ValTy_*)p_void;
    }
    void restore(archive::if_reader* reader, void* p_void) const override
    {
        *reader >> *(ValTy_*)p_void;
    }
};

template <typename ValTy_>
auto get_object_descriptor() -> object_sfinae_t<
        (is_any_of_v<ValTy_, bool, nullptr_t, std::string, binary_t>)
        || (std::is_integral_v<ValTy_>)
        || (std::is_floating_point_v<ValTy_>)  //
        >
{
    static struct manip_t : primitive_manipulator<ValTy_>
    {
        primitive_t type() const noexcept override
        {
            if constexpr (std::is_integral_v<ValTy_>) { return primitive_t::integer; }
            if constexpr (std::is_floating_point_v<ValTy_>) { return primitive_t::floating_point; }
            if constexpr (std::is_same_v<ValTy_, nullptr_t>) { return primitive_t::null; }
            if constexpr (std::is_same_v<ValTy_, bool>) { return primitive_t::boolean; }
            if constexpr (std::is_same_v<ValTy_, std::string>) { return primitive_t::string; }
            if constexpr (std::is_same_v<ValTy_, binary_t>) { return primitive_t::binary; }

            return primitive_t::invalid;
        }
    } manip;

    static object_descriptor desc = [] {
        object_descriptor::primitive_factory factory;
        factory.setup(sizeof(ValTy_), [] { return &manip; });
        return factory.create();
    }();

    return &desc;
}

}  // namespace CPPHEADERS_NS_::refl
