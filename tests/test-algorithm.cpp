#include <algorithm/base64.hxx>
#include <archiving/helper_macros.hxx>
#include <counter.hxx>
#include <dynamic_array.hxx>

#include "doctest.h"

using namespace std::literals;

template <size_t N_>
static void identical(char const (&S)[N_], std::string_view enc) {
  std::string_view str{S};
  auto encoded{std::array<char, cpph::base64::encoded_size(N_ - 1)>{}};

  cpph::base64::encode(str, encoded.begin());

  auto decoded = cpph::dynamic_array<char>(cpph::base64::decoded_size(encoded));
  CHECK(cpph::base64::decode(encoded, decoded.begin()));

  INFO(""
       << "\n\tsource:            " << S
       << "\n\tencoded:           " << std::string_view(encoded.data(), encoded.size())
       << "\n\tencoded should be: " << enc
       << "\n\tdecoded:           " << std::string_view(decoded.data(), decoded.size())
       << "\n\tdecoded should be: " << str);

  CHECK(std::equal(std::begin(enc), std::end(enc), encoded.begin(), encoded.end()));
  CHECK(std::equal(std::begin(str), std::end(str), decoded.begin(), decoded.end()));
}

TEST_SUITE("base64") {
  TEST_CASE("verify") {
    identical("E1L",
              "RTFM");

    identical("fsadvcxlwerlwajkrlsjbl;afaewrqweqsa12321ewq",
              "ZnNhZHZjeGx3ZXJsd2Fqa3Jsc2pibDthZmFld3Jxd2Vxc2ExMjMyMWV3cQ==");

    identical("cvxzvsdafwea",
              "Y3Z4enZzZGFmd2Vh");

    identical("lkqwlem1284v.,zㅊㄴㅁ213s1",
              "bGtxd2xlbTEyODR2Lix644WK44S044WBMjEzczE=");
  }
}

#if __has_include("nlohmann/json.hpp")

TEST_SUITE("json-macro-helper") {
  struct my_serialized {
    std::string s;
    std::optional<int> k;

    CPPHEADERS_DEFINE_ARCHIVE_METHODS(
            my_serialized,
            s,
            k);
  };

  struct k {
    int dd;
  };

  TEST_CASE("verify") {
    using namespace cpph::archiving;
    static constexpr char cands[] = "a, dd,  vc, fewa , rq_w1141";
    static_assert(detail::_count_words<cands>() == 5);

    static_assert(detail::break_VA_ARGS<cands>()[0] == "a"sv);
    static_assert(detail::break_VA_ARGS<cands>()[1] == "dd"sv);
    static_assert(detail::break_VA_ARGS<cands>()[2] == "vc"sv);
    static_assert(detail::break_VA_ARGS<cands>()[3] == "fewa"sv);
    static_assert(detail::break_VA_ARGS<cands>()[4] == "rq_w1141"sv);

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
