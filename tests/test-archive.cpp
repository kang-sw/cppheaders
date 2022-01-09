#include "refl/archive/debug_string_writer.hxx"
#include "refl/if_archive.hxx"
#include "refl/object.hxx"
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

    return &instance;
}

template <class T>
auto get_object_descriptor()
        -> cpph::refl::object_sfinae_t<std::is_same_v<T, test_object_of_object>>
{
    static auto instance = [] {
        return object_descriptor::object_factory::define<test_object_of_object>()
                .define_basic(sizeof(test_object_of_object))
                .add_property("a", {offsetof(test_object_of_object, a), default_object_descriptor_fn<test_object>()})
                .add_property("b", {offsetof(test_object_of_object, b), default_object_descriptor_fn<test_object>()})
                .add_property("c", {offsetof(test_object_of_object, c), default_object_descriptor_fn<test_object>()})
                .create();
    }();

    return &instance;
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

    return &instance;
}

}  // namespace cpph::refl

TEST_SUITE("Reflection")
{
    TEST_CASE("Creation")
    {
        {
            auto desc = perfkit::refl::get_object_descriptor<test_object>();
            REQUIRE(desc->properties().size() == 3);
            REQUIRE(desc->is_object());
            REQUIRE(desc->extent() == sizeof(test_object));
        }
        {
            auto desc = perfkit::refl::get_object_descriptor<test_object_of_object>();
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
    }
}
