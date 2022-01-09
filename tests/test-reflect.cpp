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

#include "catch.hpp"
#include "refl/buffer.hxx"
#include "refl/if_archive.hxx"
#include "refl/object_core.hxx"

struct test_object
{
    int a = 1;
    int b = 2;
    int c = 3;
};

struct test_macro_expr_3
{
    int a = 01;
    int b = 4;
    int c = 5;

    cpph::refl::object_descriptor_ptr
    cpph_refl_get_object_descriptor() const noexcept;
};

struct test_object_of_object
{
    test_object a;
    test_object b;
    test_object c;
};

struct test_tuple
{
    test_object_of_object a, b, c;
};

struct test_macro_expr_1
{
    test_tuple a, b, c;
};

struct test_macro_expr_2
{
    test_tuple a, b, c;
};

struct test_tuple_2
{
    test_tuple a, b, c;
};

struct test_object_2
{
    test_tuple a, b, c;
};

CPPH_REFL_DECLARE(test_object);
CPPH_REFL_DECLARE(test_object_of_object);
CPPH_REFL_DECLARE(test_tuple);
CPPH_REFL_DECLARE(test_tuple_2);
CPPH_REFL_DECLARE(test_object_2);
CPPH_REFL_DECLARE(test_macro_expr_1);
CPPH_REFL_DECLARE(test_macro_expr_2);

TEST_CASE("creation", "[reflection]")
{
    cpph::refl::get_object_descriptor<test_macro_expr_3>();
    {
        auto desc = cpph::refl::get_object_descriptor<test_object>();
        REQUIRE(desc->properties().size() == 3);
        REQUIRE(desc->is_object());
        REQUIRE(desc->extent() == sizeof(test_object));

        auto prop = desc->property("b");
    }
    {
        auto desc = cpph::refl::get_object_descriptor<test_object_of_object>();
        REQUIRE(desc->properties().size() == 3);
        REQUIRE(desc->is_object());
        REQUIRE(desc->extent() == sizeof(test_object_of_object));
    }
    {
        auto desc = perfkit::refl::get_object_descriptor<test_tuple>();
        REQUIRE(desc->properties().size() == 3);
        REQUIRE(desc->is_tuple());
        REQUIRE(desc->extent() == sizeof(test_tuple));
    }
    {
        auto desc = perfkit::refl::get_object_descriptor<test_macro_expr_1>();
        REQUIRE(desc->properties().size() == 3);
        REQUIRE(desc->is_object());
        REQUIRE(desc->extent() == sizeof(test_macro_expr_1));
    }
    {
        auto desc = perfkit::refl::get_object_descriptor<test_macro_expr_2>();
        REQUIRE(desc->properties().size() == 3);
        REQUIRE(desc->is_object());
        REQUIRE(desc->extent() == sizeof(test_macro_expr_1));
    }
    {
        auto desc = perfkit::refl::get_object_descriptor<test_macro_expr_3>();
        REQUIRE(desc->properties().size() == 1);
        REQUIRE(desc->is_object());
        REQUIRE(desc->extent() == sizeof(test_macro_expr_3));
    }
}

#include "refl/object.hxx"
namespace cpph::refl {
template <class T>
auto get_object_descriptor()
        -> cpph::refl::object_sfinae_t<std::is_same_v<T, test_object>>
{
    static auto instance = [] {
        return define_object<test_object>()
                .property("a", &test_object::a)
                .property("b", &test_object::b)
                .property("c", &test_object::c)
                .create();
    }();

    return &*instance;
}

template <class T>
auto get_object_descriptor()
        -> cpph::refl::object_sfinae_t<std::is_same_v<T, test_object_of_object>>
{
    static auto instance = [] {
        return object_descriptor::object_factory()
                .define_basic(sizeof(test_object_of_object))
                .add_property("a", {offsetof(test_object_of_object, a), default_object_descriptor_fn<test_object>()})
                .add_property("b", {offsetof(test_object_of_object, b), default_object_descriptor_fn<test_object>()})
                .add_property("c", {offsetof(test_object_of_object, c), default_object_descriptor_fn<test_object>()})
                .create();
    }();

    return &*instance;
}

template <class T>
auto get_object_descriptor()
        -> cpph::refl::object_sfinae_t<std::is_same_v<T, test_tuple>>
{
    static auto instance = [] {
        return define_tuple<test_tuple>()
                .property(&test_tuple::a)
                .property(&test_tuple::b)
                .property(&test_tuple::c)
                .create();
    }();

    return &*instance;
}

}  // namespace cpph::refl

using ClassName = test_macro_expr_1;

using INTERNAL_CPPH_CONCAT(ClassName__Type, LINE__) = ClassName;
extern CPPHEADERS_NS_::refl::descriptor_generate_fn INTERNAL_CPPH_CONCAT(ClassName, LINE__);

namespace CPPHEADERS_NS_::refl {

template <class TypeName_>
auto get_object_descriptor()
        -> object_sfinae_t<std::is_same_v<TypeName_, INTERNAL_CPPH_CONCAT(ClassName__Type, LINE__)>>
{
    static auto instance = INTERNAL_CPPH_CONCAT(ClassName, LINE__)();
    return &*instance;
}

}  // namespace CPPHEADERS_NS_::refl

CPPHEADERS_NS_::refl::descriptor_generate_fn INTERNAL_CPPH_CONCAT(ClassName, LINE__)
        = [ptr          = (INTERNAL_CPPH_CONCAT(ClassName__Type, LINE__) *)nullptr,
           Registration = CPPHEADERS_NS_::refl::define_object<INTERNAL_CPPH_CONCAT(ClassName__Type, LINE__)>()] {
              return Registration
                      .property("hello", &std::remove_pointer_t<decltype(ptr)>::a)
                      .property("hello-2", &std::remove_pointer_t<decltype(ptr)>::b)
                      .property("hello-3", &std::remove_pointer_t<decltype(ptr)>::c)
                      .create();
          };

#define property_ CPPH_PROP_OBJECT_AUTOKEY
CPPH_REFL_DEFINE_OBJECT(test_macro_expr_2)
{
    return factory
            .property_(a)
            .property_(b)
            .property_(c)
            .create();
};

cpph::refl::object_descriptor_ptr test_macro_expr_3::cpph_refl_get_object_descriptor() const noexcept
{
    using self_t = std::remove_pointer_t<decltype(this)>;
    size_t pos   = size_t((char const*)&a - (char const*)this);

    return perfkit::refl::define_object<test_macro_expr_3>()
            .property("hello", &self_t::a)
            .create();
}
