#include <base64.hxx>
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
