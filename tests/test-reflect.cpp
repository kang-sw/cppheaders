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
#include "helper/macro_for_each.hxx"
#include "refl/archive.hxx"
#include "refl/buffer.hxx"
#include "refl/object_core.hxx"

namespace my_ns {
struct test_object_1
{
    int a, b, c, d;
};

struct test_object_2
{
    int a, b, c, d;
};

CPPH_REFL_DECLARE(test_object_1);
static auto const ptr = cpph::refl::get_object_metadata<test_object_1>();

}  // namespace my_ns

TEST_CASE("macro test", "[.]")
{
    CPPH_FOR_EACH(puts, "ha", "he");
}

#include "refl/refl.hxx"
namespace my_ns {
CPPH_REFL_DEFINE_OBJECT(test_object_1, (a), (b), (c), (d));
}
