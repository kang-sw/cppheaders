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

#include "catch.hpp"
#include "streambuf/base64.hxx"
#include "streambuf/view.hxx"

using namespace cpph;

TEST_CASE("base64 feature test", "[streambuf]")
{
    std::string str = "hello, world! 0abcdefg_1234_HZZEE";
    streambuf::basic_b64 b64{};

    std::stringstream buffer;

    b64.reset(buffer.rdbuf());
    b64.sputn(str.data(), str.size());
    b64.pubsync();
    REQUIRE(buffer.str() == "aGVsbG8sIHdvcmxkISAwYWJjZGVmZ18xMjM0X0haWkVF");

    std::stringstream binbuf;
    binbuf << &b64;

    REQUIRE(binbuf.str() == str);
}
