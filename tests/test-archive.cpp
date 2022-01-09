#include "refl/archive/debug_string_writer.hxx"
#include "refl/if_archive.hxx"
#include "refl/object_core.hxx"
#include "third/doctest.h"

struct test_object
{
    int a = 1;
    int b = 2;
    int c = 3;
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

TEST_SUITE("Reflection")
{
    TEST_CASE("Creation")
    {
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

INTERNAL_CPPH_DEFINE_IMPL(test_macro_expr_2, factory, CPPHEADERS_NS_::refl::define_object)
{
#define property_ CPPH_prop_2
    return factory
            .property_(a)
            .property_(b)
            .property_(c)
            .create();
#undef property_
};
