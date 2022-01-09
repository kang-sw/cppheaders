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
#include "refl/archive/debug_string_writer.hxx"
#include "refl/buffer.hxx"
#include "refl/object.hxx"

using namespace cpph;

namespace ns {
struct inner_arg_1
{
    std::string str1 = "str1";
    std::string str2 = "str2";
    int var          = 133;
    bool k           = true;
    double g         = 3.14;
};

struct inner_arg_2
{
    inner_arg_1 rtt    = {};
    nullptr_t nothing  = nullptr;
    nullptr_t nothing2 = nullptr;

    int ints[3] = {1, 23, 4};
};

struct outer
{
    inner_arg_1 arg1;
    inner_arg_2 arg2;
};
}  // namespace ns

CPPH_REFL_DECLARE(ns::inner_arg_1);
CPPH_REFL_DECLARE(ns::inner_arg_2);
CPPH_REFL_DECLARE(ns::outer);

TEMPLATE_TEST_CASE("archive", "[.]", ns::inner_arg_1, ns::inner_arg_2, ns::outer)
{
    archive::debug_string_writer writer{archive::obuffer(std::cout)};

    std::cout << "\n\n------- CLASS " << typeid(TestType).name() << " -------\n\n";
    writer.serialize(TestType{});

    std::cout.flush();
}

#define property_  CPPH_PROP_TUPLE
#define property2_ CPPH_PROP_OBJECT_AUTOKEY
CPPH_REFL_DEFINE_TUPLE(ns::inner_arg_1)
{
    return factory
            .property_(str1)
            .property_(str2)
            .property_(var)
            .property_(k)
            .property_(g)
            .create();
};

CPPH_REFL_DEFINE_OBJECT(ns::inner_arg_2)
{
    return factory
            .property2_(nothing)
            .property2_(rtt)
            .property2_(nothing2)
            .property2_(ints)
            .create();
};

CPPH_REFL_DEFINE_OBJECT(ns::outer)
{
    return factory
            .property2_(arg1)
            .property2_(arg2)
            .create();
};
