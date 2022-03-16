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

#include <chrono>
#include <memory>

#include <functional.hxx>

#include "catch.hpp"

TEST_CASE("overall operation", "[functional]")
{
    cpph::function<int(void)> s;
    s = [] {
        return 1;
    };

    CHECK(s() == 1);
    CHECK(s.is_sbo());

    std::unique_ptr<int> ptr{new int{3}};
    s = [ptr = std::move(ptr)] {
        return *ptr;
    };

    CHECK(s() == 3);
    CHECK(s.is_sbo());

    auto d = std::move(s);

    CHECK(d() == 3);
    CHECK(d.is_sbo());

    s = std::function<int(void)>{
            [] {
                return 444;
            }};

    CHECK(s.is_sbo());
    CHECK(s() == 444);
}

struct lambda_t
{
    std::array<char, 40> bf;

    void                 operator()()
    {
        volatile int s = 0;
        s              = 1;
        s              = 0;
    }
};

TEST_CASE("benchmark")
{
    return;

    cpph::function<void()> test1 = lambda_t{};
    std::function<void()>  test2 = lambda_t{};

    using clk                    = std::chrono::system_clock;

    auto n1                      = clk::now();
    for (auto i = 0; i < 400'000; ++i) {
        test1 = lambda_t{};
        test1();
    }
    auto n2 = clk::now();

    for (auto i = 0; i < 400'000; ++i) {
        test2 = lambda_t{};
        test2();
    }
    auto n3  = clk::now();

    auto t_1 = n2 - n1;
    auto t_2 = n2 - n1;

    INFO("..."
         << '\n'
         << "CPPH FUNCTION: " << std::chrono::duration<double>(t_1).count() << '\n'
         << "STD  FUNCTION: " << std::chrono::duration<double>(t_2).count());
}

TEST_CASE("release")
{
    auto                  s = std::make_shared<int>();
    std::weak_ptr<int>    w = s;

    cpph::function<int()> f{
            [s = std::move(s)] {
                return s.use_count();
            }};

    REQUIRE(f() == 1);

    SECTION("iter 0")
    {
        f = {};

        CHECK(w.expired());
    }

    SECTION("iter 1")
    {
        auto g = std::move(f);

        CHECK(g() == 1);
        g = {};

        CHECK(w.expired());
    }
}

template <size_t N>
class la;
