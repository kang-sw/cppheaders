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

// H
#include <iostream>
#include <list>
#include <optional>
#include <sstream>

#include "refl/core.hxx"

struct base_object {
    std::optional<double> opt_double;
    std::list<int> list_int;
    cpph::binary<std::vector<char>> bin_vec_chars;

    auto& fill()
    {
        opt_double = 31314.;
        list_int = {1, 2, 34};
        bin_vec_chars.assign({'h', 'e', 'l', 'l', 'o'});

        return *this;
    }
};

struct child_object : base_object {
    CPPH_REFL_DECLARE_c;

    nullptr_t placeholder;
};

CPPH_REFL_DECLARE(base_object);

// CPP

#include "catch.hpp"
#include "refl/archive/debug_string_writer.hxx"
#include "refl/archive/json.hpp"
#include "refl/archive/msgpack-reader.hxx"
#include "refl/archive/msgpack-writer.hxx"
#include "refl/object.hxx"
#include "refl/types/array.hxx"
#include "refl/types/binary.hxx"
#include "refl/types/chrono.hxx"
#include "refl/types/key.hxx"
#include "refl/types/list.hxx"
#include "refl/types/variant.hxx"

using namespace cpph;

CPPH_REFL_DEFINE_OBJECT(base_object, (), (opt_double), (list_int), (bin_vec_chars));
CPPH_REFL_DEFINE_OBJECT_c(child_object, (.extend<base_object>()), (placeholder));

using test_templ_type_1 = std::pair<archive::json::writer, archive::json::reader>;
using test_templ_type_2 = std::pair<archive::msgpack::writer, archive::msgpack::reader>;

TEST_SUITE("refl.archive")
{
    TEST_CASE_TEMPLATE("marshalling, deserialize", TestType, test_templ_type_1, test_templ_type_2)
    {
        using writer = typename TestType::first_type;
        using reader = typename TestType::second_type;

        for (int intkey = 0; intkey < 2; ++intkey) {
            std::stringstream sstrm;

            writer wr{sstrm.rdbuf()};
            wr.config.use_integer_key = intkey;

            child_object enc{};
            enc.fill();
            wr << enc;
            auto content_1 = sstrm.str();
            INFO(content_1);

            child_object b;
            reader rd{sstrm.rdbuf()};
            rd.config.use_integer_key = intkey;
            rd >> b;

            sstrm.str(""), wr << b;
            auto content_2 = sstrm.str();

            REQUIRE(content_1 == content_2);
        }
    }

    TEST_CASE("Retrieve throw exceptions")
    {
        std::stringbuf sbuf;
        archive::msgpack::writer writer{&sbuf};
        archive::msgpack::reader reader{&sbuf};

        SUBCASE("Tolerant")
        {
            SUBCASE("Child to base")
            {
                writer << child_object{};

                base_object target;
                REQUIRE_NOTHROW(reader >> target);
            }

            SUBCASE("Base to child")
            {
                writer << base_object{};

                child_object child;
                REQUIRE_NOTHROW(reader >> child);
            }
        }

        SUBCASE("No Missing")
        {
            reader.config.allow_missing_argument = false;

            SUBCASE("Child to base")
            {
                writer << child_object{};

                base_object target;
                REQUIRE_NOTHROW(reader >> target);
            }

            SUBCASE("Base to child")
            {
                writer << base_object{};

                child_object child;
                REQUIRE_THROWS_AS(reader >> child, refl::error::missing_entity);
            }
        }

        SUBCASE("No Unknown")
        {
            reader.config.allow_unknown_argument = false;

            SUBCASE("Child to base")
            {
                writer << child_object{};

                base_object target;
                REQUIRE_THROWS_AS(reader >> target, refl::error::unkown_entity);
            }

            SUBCASE("Base to child")
            {
                writer << base_object{};

                child_object child;
                REQUIRE_NOTHROW(reader >> child);
            }
        }
    }
}
