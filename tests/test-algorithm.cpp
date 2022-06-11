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

#include <algorithm/base64.hxx>
#include <container/dynamic_array.hxx>
#include <helper/nlohmann_json_macros.hxx>
#include <utility/counter.hxx>

#include "catch.hpp"

using namespace std::literals;

template <size_t N_>
static void identical(char const (&S)[N_], std::string_view enc)
{
    std::string_view str{S};
    auto encoded{std::array<char, cpph::base64::encoded_size(N_ - 1)>{}};

    cpph::base64::encode(str, encoded.begin());

    auto decoded = cpph::dynamic_array<char>(cpph::base64::decoded_size(encoded));
    CHECK(cpph::base64::decode(encoded, decoded.begin()));

    //    INFO(""
    //         << "\n\tsource:            " << S
    //         << "\n\tencoded:           " << std::string_view(encoded.data(), encoded.size())
    //         << "\n\tencoded should be: " << enc
    //         << "\n\tdecoded:           " << std::string_view(decoded.data(), decoded.size())
    //         << "\n\tdecoded should be: " << str);

    CHECK(std::equal(std::begin(enc), std::end(enc), encoded.begin(), encoded.end()));
    CHECK(std::equal(std::begin(str), std::end(str), decoded.begin(), decoded.end()));
}

TEST_SUITE("misc")
{
    TEST_CASE("base64 correctly converted")
    {
        identical("E1L",
                  "RTFM");

        identical("fsadvcxlwerlwajkrlsjbl;afaewrqweqsa12321ewq",
                  "ZnNhZHZjeGx3ZXJsd2Fqa3Jsc2pibDthZmFld3Jxd2Vxc2ExMjMyMWV3cQ==");

        identical("cvxzvsdafwea",
                  "Y3Z4enZzZGFmd2Vh");

        // disable due to encoding
        //        identical("lkqwlem1284v.,zㅊㄴㅁ213s1",
        //                  "bGtxd2xlbTEyODR2Lix644WK44S044WBMjEzczE=");
    }
}

#if __has_include("nlohmann/json.hpp")

struct my_serialized {
    std::string s;
    std::optional<int> k;

    CPPHEADERS_DEFINE_NLOHMANN_JSON_ARCHIVER(
            my_serialized,
            s,
            k);
};

TEST_SUITE("misc")
{
    TEST_CASE("json macro helper operation")
    {
        struct k {
            int dd;
        };

        using namespace cpph::macro_utils;
        static constexpr char cands[] = "a, dd,  vc, fewa , rq_w1141";
        static_assert(_count_words<cands>() == 5);

        static_assert(break_VA_ARGS<cands>()[0] == "a"sv);
        static_assert(break_VA_ARGS<cands>()[1] == "dd"sv);
        static_assert(break_VA_ARGS<cands>()[2] == "vc"sv);
        static_assert(break_VA_ARGS<cands>()[3] == "fewa"sv);
        static_assert(break_VA_ARGS<cands>()[4] == "rq_w1141"sv);

        my_serialized a;
        a.s = "hello!";

        nlohmann::json v;
        REQUIRE_NOTHROW(v = a);
        REQUIRE(v.contains("s"));
        CHECK(v.at("s") == "hello!");

        CHECK(not v.contains("k"));

        a.k = 14;
        REQUIRE_NOTHROW(v = a);

        REQUIRE(v.contains("k"));
        CHECK(v.at("k") == 14);

        my_serialized b;

        v = {};
        REQUIRE_THROWS(v.get_to(b));

        v["s"] = "vvarr";
        REQUIRE_NOTHROW(v.get_to(b));
        CHECK(b.s == "vvarr");

        v["k"] = 1;
        REQUIRE_NOTHROW(v.get_to(b));
        CHECK(b.s == "vvarr");
        CHECK(b.k == 1);
    }
}
#endif
