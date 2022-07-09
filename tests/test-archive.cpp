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
#include "refl/archive/msgpack-reader.hxx"
#include "refl/archive/msgpack-writer.hxx"
#include "refl/object.hxx"
#include "refl/types/array.hxx"
#include "refl/types/binary.hxx"
#include "refl/types/list.hxx"
#include "refl/types/tuple.hxx"
#include "refl/types/variant.hxx"

using namespace cpph;

enum class my_enum {
    test1,
    test2,
    test3
};

namespace ns {
struct inner_arg_1 {
    std::string str1 = (char const*)u8"str1-value\r\t\n\\n";
    std::string str2 = "str2-value";
    int var = 133;
    bool k = true;
    std::array<bool, 4> bools = {false, false, true, false};
    double g = 3.14;

    CPPH_REFL_DEFINE_OBJECT_inline((), (str1), (str2),
                                   (var), (k), (bools), (g));
};

struct inner_arg_2 {
    inner_arg_1 rtt = {};
    nullptr_t nothing = nullptr;
    nullptr_t nothing2 = nullptr;

    int ints[3] = {1, 23, 4};

    CPPH_REFL_DEFINE_TUPLE_inline((), rtt, nothing, nothing2, ints);
};

struct abcd {
    int arg0 = 1;
    int arg1 = 2;
    int arg2 = 3;
    int arg3 = 4;

    CPPH_REFL_DEFINE_OBJECT_inline((), (arg0), (arg1), (arg2), (arg3));
};

struct outer {
    inner_arg_1 arg1;
    inner_arg_2 arg2;
    std::pair<int, bool> arg = {3, false};
    std::tuple<int, double, std::string> bb = {5, 1.14, "hello"};

    binary<abcd> r;

    std::map<std::string, abcd> afs = {
            {"aa", {1, 2, 3, 4}},
            {"bb", {1, 3, 2, 5}},
    };

    std::unique_ptr<int> no_value;
    std::unique_ptr<int> has_value = std::make_unique<int>(3);
    std::shared_ptr<int> has_value_s = std::make_shared<int>(3);

    CPPH_REFL_DEFINE_OBJECT_inline((), (arg1),
                                   (arg2, "afd"), (arg, 1), (bb, "gcd", 13), (r), (afs), (no_value), (has_value), (has_value_s));
};

template <typename S, typename T>
class values_0
{
   public:
    S a, b, c, d, e, f;
};

template <typename S, typename T>
auto initialize_object_metadata(cpph::refl::type_tag<values_0<S, T>>)
{
    using Self = values_0<S, T>;

    return cpph::refl::define_object<Self>()
            .property(&Self::a, "a")
            .property(&Self::b, "b", 3)
            .property(&Self::c, "c")
            .property(&Self::d, "d", 5)
            .property(&Self::e, "e")
            .property(&Self::f, "f", 18)
            .create();
}

static auto pp_ptr = refl::get_object_metadata<values_0<int, double>>();
static_assert(is_binary_compatible_v<abcd>);

struct some_other {
    int a, b, c;
    float f, t, r;

    outer e;
    inner_arg_2 ff;

    CPPH_REFL_DECLARE_c;
};

using variant_type = std::variant<int, double, std::string, bool>;

struct vectors {
    std::vector<std::vector<double>> f
            = {{1., 2., 3.}, {4., 5, 6}};

    std::vector<std::list<double>> f2
            = {{1., 2., 3.}, {4., 5, 6}};

    binary<std::vector<int>> f3{1, 2, 3, 4};
    binary<std::list<int>> f4{0x5abbccdd, 0x12213456, 0x31315142};
    binary<abcd> f5;

    my_enum my_enum_value = my_enum::test3;

    std::pair<int, bool> arg = {3, false};
    std::tuple<int, double, std::string> bb = {5, 1.14, "hell?금?방?갈?게?요?o"};

    outer some_outer;

    std::optional<int> no_val;
    std::optional<int> has_val = 1;

    variant_type vt1 = 3;
    variant_type vt2 = 3.14;
    variant_type vt3 = std::string{"hello!"};
    variant_type vt4 = false;

    CPPH_REFL_DEFINE_OBJECT_inline((), (bb, "BB", 0x7fffffff),
                                   (f), (f2), (f3), (f4), (f5), (my_enum_value), (arg), (some_outer),
                                   (no_val), (has_val),
                                   (vt1), (vt2), (vt3), (vt4));
};

struct some_other_2 {
    int a, b, c;
    float f, t, r;

    outer e;
    inner_arg_2 ff;

    CPPH_REFL_DECLARE_em(some_other_2);
};

}  // namespace ns

namespace cpph::refl {

}

struct testarg_2 {
    std::string unistr;

    CPPH_REFL_DEFINE_OBJECT_inline((), (unistr));
};

std::string g_debugstr_1;
std::string g_debugstr_2;

class stream_debug_adapter : public std::streambuf
{
    std::streambuf* _other;
    std::string* _str;

   public:
    stream_debug_adapter(std::streambuf& other, std::string* str) : _other(&other), _str(str) {}

   protected:
    int_type overflow(int_type ch) override
    {
        fputc(ch, stdout), fflush(stdout);
        _str->push_back(ch);
        return _other->sputc(ch);
    }

    int_type underflow() override
    {
        auto ch = _other->sgetc();
        return ch;
    }

    int_type uflow() override
    {
        auto ch = _other->sbumpc();
        fputc(ch, stdout), fflush(stdout);
        _str->push_back(ch);

        return ch;
    }
};

static auto ssvd = [] {
    using TestType = ns::vectors;
    std::stringbuf strbuf;
    archive::json::writer writer{&strbuf};

    TestType arg{};

    std::cout << "\n\n------- CLASS " << typeid(TestType).name() << " -------\n\n";
    writer << arg;
    std::cout << strbuf.str();
    std::cout.flush();

    std::cout << "\n\n------- CLASS INTKEY << " << typeid(TestType).name() << " -------\n\n";
    std::stringbuf strbuf_intkey;
    archive::json::writer writer_intkey{&strbuf_intkey};
    writer_intkey.config.use_integer_key = true;
    writer_intkey << arg;
    std::cout << strbuf_intkey.str() << std::endl;

    std::cout << "\n\n------- CLASS INTKEY >> " << typeid(TestType).name() << " -------\n\n";
    archive::json::reader reader_intkey{&strbuf_intkey};
    reader_intkey.config.use_integer_key = true;
    reader_intkey >> arg;
    writer_intkey.rdbuf(std::cout.rdbuf());
    writer_intkey << arg;

    std::cout << "\n\n------- PARSE " << typeid(TestType).name() << " -------\n\n";
    archive::json::reader reader{&strbuf};
    TestType other;
    reader.deserialize(other);

    archive::json::writer wr2{std::cout.rdbuf()};
    wr2.serialize(other);
    std::cout << "\n\n------- DONE  " << typeid(TestType).name() << " -------\n\n";

    writer << std::string_view{"hello"};
    std::string str;
    reader >> str;

    std::stringstream msgpack_bufb64;
    streambuf::b64 cvtbase64{msgpack_bufb64.rdbuf()};

    archive::msgpack::writer msgwr{&cvtbase64};
    msgwr.config.use_integer_key = true;
    msgwr << TestType{};

    cvtbase64.pubsync();
    std::cout << msgpack_bufb64.str();
    std::cout << "\n----------- MSGPACK READING -------------- " << std::endl;

    archive::msgpack::reader msgrd{&cvtbase64};
    msgrd.config.use_integer_key = true;

    TestType other2{};
    other2.arg = {};
    other2.bb = {};
    other2.f = {};
    other2.has_val = {};

    g_debugstr_1.clear();
    msgrd >> other2;

    wr2 << other2;

    return nullptr;
};

TEST_SUITE("refl.archive")
{
    TEST_CASE("archive")
    {
        ssvd();

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

    TEST_CASE("archive.json.goto_key")
    {
        auto str = archive::to_json(ns::vectors{});
        streambuf::view buf_view{str};
        archive::json::reader reader{&buf_view};

        auto key = reader.begin_object();
        REQUIRE(reader.goto_key("vt1"));
        REQUIRE(reader.is_array_next());
        {
            ns::variant_type t;
            REQUIRE_NOTHROW(reader >> t);
        }

        REQUIRE(reader.goto_key("f"));
        REQUIRE(reader.is_array_next());
        {
            std::vector<std::vector<double>> gg;
            REQUIRE_NOTHROW(reader >> gg);
        }

        REQUIRE(reader.goto_key("my_enum_value"));
        REQUIRE(reader.type_next() == cpph::archive::entity_type::integer);
        {
            int g;
            REQUIRE_NOTHROW(reader >> g);
        }

        REQUIRE(reader.goto_key("some_outer"));
        REQUIRE(reader.is_object_next());
        {
            ns::outer g;
            REQUIRE_NOTHROW(reader >> g);
        }

        reader.end_object(key);
    }
}

#define property_  CPPH_PROP_TUPLE
#define property2_ CPPH_PROP_OBJECT_AUTOKEY

// cpph::refl::unique_object_metadata
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
CPPH_REFL_DEFINE_OBJECT_c(ns::some_other, (), (a, "hello", 3), (b, 4), (c), (f), (t), (r), (e), (ff));
CPPH_REFL_DEFINE_TUPLE(ns::some_other_2, (), a, b, c, f, t, r, e, ff);

struct bintest {
    binary<std::string> binstr = "hello, world!";

    CPPH_REFL_DEFINE_OBJECT_inline((), (binstr));
};

TEST_SUITE("refl.archive")
{
    TEST_CASE("base64 restoration")
    {
        std::stringbuf strbuf;
        archive::json::writer writer{&strbuf};
        writer.indent = 4;

        writer.serialize(bintest{});

        INFO("---- ARCHIVED ----\n"
             << strbuf.str());

        bintest restored;
        restored.binstr = {};

        archive::json::reader reader{&strbuf};
        reader.deserialize(restored);

        INFO("---- RESTORED.binstr ----\n"
             << restored.binstr);

        REQUIRE(restored.binstr == bintest{}.binstr);
    }

    TEST_CASE("object view get ptr")
    {
        refl::shared_object_ptr p{make_shared<int>(4)};

        REQUIRE(refl::get_ptr<int>(p) != nullptr);
        REQUIRE(*refl::get_ptr<int>(p) == 4);
    }
}
