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

#include <iostream>
#include <sstream>
#include <variant>

#include "catch.hpp"
#include "refl/archive/debug_string_writer.hxx"
#include "refl/archive/json.hpp"
#include "refl/buffer.hxx"
#include "refl/container/variant.hxx"
#include "refl/object.hxx"

using namespace cpph;

enum class my_enum
{
    test1,
    test2,
    test3
};

namespace ns {
struct inner_arg_1
{
    std::string str1          = (char const*)u8"str1-value\r\t\n\\n";
    std::string str2          = "str2-value";
    int var                   = 133;
    bool k                    = true;
    std::array<bool, 4> bools = {false, false, true, false};
    double g                  = 3.14;

    CPPH_REFL_DEFINE_OBJECT_inline(
            str1, str2,
            var, k, bools, g);
};

struct inner_arg_2
{
    inner_arg_1 rtt    = {};
    nullptr_t nothing  = nullptr;
    nullptr_t nothing2 = nullptr;

    int ints[3] = {1, 23, 4};

    CPPH_REFL_DEFINE_TUPLE_inline(rtt, nothing, nothing2, ints);
};

struct abcd
{
    int arg0 = 1;
    int arg1 = 2;
    int arg2 = 3;
    int arg3 = 4;

    CPPH_REFL_DEFINE_OBJECT_inline(arg0, arg1, arg2, arg3);
};

struct outer
{
    inner_arg_1 arg1;
    inner_arg_2 arg2;
    std::pair<int, bool> arg                = {3, false};
    std::tuple<int, double, std::string> bb = {5, 1.14, "hello"};

    perfkit::binary<abcd> r;
    std::map<std::string, abcd> afs = {
            {"aa", {1, 2, 3, 4}},
            {"bb", {1, 3, 2, 5}},
    };

    std::unique_ptr<int> no_value;
    std::unique_ptr<int> has_value   = std::make_unique<int>(3);
    std::shared_ptr<int> has_value_s = std::make_shared<int>(3);

    CPPH_REFL_DEFINE_TUPLE_inline(arg1, arg2, arg, bb, afs, no_value, has_value, has_value_s);
};

template <typename S, typename T>
class Values
{
    S a, b, c;
};

template <typename S, typename T>
cpph::refl::object_metadata_ptr
initialize_object_metadata(cpph::refl::type_tag<Values<S, T>>)
{
    return {};
}

static auto pp_ptr = refl::get_object_metadata<Values<int, double>>();
static_assert(perfkit::is_binary_compatible_v<abcd>);

struct some_other
{
    int a, b, c;
    float f, t, r;

    outer e;
    inner_arg_2 ff;

    CPPH_REFL_DECLARE_c;
};

using variant_type = std::variant<int, double, std::string, bool>;

struct vectors
{
    std::vector<std::vector<double>> f
            = {{1., 2., 3.}, {4., 5, 6}};

    std::vector<std::list<double>> f2
            = {{1., 2., 3.}, {4., 5, 6}};

    cpph::binary<std::vector<int>> f3{1, 2, 3, 4};
    cpph::binary<std::list<int>> f4{0x5abbccdd, 0x12213456, 0x31315142};

    my_enum my_enum_value = my_enum::test3;

    std::pair<int, bool> arg                = {3, false};
    std::tuple<int, double, std::string> bb = {5, 1.14, "hell금방갈게요o"};

    outer some_outer;

    std::optional<int> no_val;
    std::optional<int> has_val = 1;

    variant_type vt1 = 3;
    variant_type vt2 = 3.14;
    variant_type vt3 = std::string{"hello!"};
    variant_type vt4 = false;

    CPPH_REFL_DEFINE_OBJECT_inline(
            f, f2, f3, f4, my_enum_value, arg, bb, some_outer,
            no_val, has_val,
            vt1, vt2, vt3, vt4);
};

struct some_other_2
{
    int a, b, c;
    float f, t, r;

    outer e;
    inner_arg_2 ff;

    CPPH_REFL_DECLARE_c;
};

}  // namespace ns

namespace cpph::refl {

}

struct testarg_2
{
    std::string unistr;

    CPPH_REFL_DEFINE_OBJECT_inline(unistr);
};

static auto ssvd = [] {
    using TestType = ns::vectors;
    std::stringbuf strbuf;
    archive::json::writer writer{strbuf};
    writer.indent = 4;
    TestType arg{};

    std::cout << "\n\n------- CLASS " << typeid(TestType).name() << " -------\n\n";
    writer.serialize(arg);
    std::cout << strbuf.str();
    std::cout.flush();

    std::cout << "\n\n------- PARSE " << typeid(TestType).name() << " -------\n\n";
    archive::json::reader reader{strbuf};
    TestType other;
    reader.deserialize(other);

    archive::json::writer wr2{*std::cout.rdbuf()};
    wr2.indent = 4;
    wr2.serialize(other);
    std::cout << "\n\n------- DONE  " << typeid(TestType).name() << " -------\n\n";

    (archive::if_writer&)writer << std::string_view{"hello"};
    std::string str;
    (archive::if_reader&)reader >> str;

    return nullptr;
}();

TEST_CASE("archive", "[.]")
{
    std::stringbuf strbuf;
    strbuf.sputn("1234", 4);

    strbuf.sbumpc();
    strbuf.sbumpc();
    strbuf.sbumpc();
    strbuf.sbumpc();

    REQUIRE(strbuf.sbumpc() == EOF);
    strbuf.sputc('4');
    REQUIRE(strbuf.sbumpc() == '4');
    REQUIRE(strbuf.sbumpc() == EOF);
}

#define property_  CPPH_PROP_TUPLE
#define property2_ CPPH_PROP_OBJECT_AUTOKEY

// cpph::refl::object_metadata_ptr
// ns::some_other::initialize_object_metadata() noexcept
//{
//     INTERNAL_CPPH_ARCHIVING_BRK_TOKENS_0("a,b,c,f,t,r")
//     auto factory = cpph::refl::define_object<std::remove_pointer_t<decltype(this)>>();
//
//     cpph::macro_utils::visit_with_key(
//             INTERNAL_CPPH_BRK_TOKENS_ACCESS_VIEW(),
//             [&](std::string_view key, auto& value) {
//                 factory.property(key, this, &value);
//             },
//             a, b, c, f, t, r);
//
//     return factory.create();
// }
CPPH_REFL_DEFINE_OBJECT_c(ns::some_other, a, b, c, f, t, r, e, ff);
CPPH_REFL_DEFINE_TUPLE_c(ns::some_other_2, a, b, c, f, t, r, e, ff);
