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

#include <future>

#include "catch.hpp"
#include "thread/local_async.hxx"

using namespace std::literals;

TEST_SUITE("thread")
{
    TEST_CASE("local_async.works well?")
    {
        using namespace cpph::thread;
        auto fut = local_task<int>();

        REQUIRE_THROWS(fut.get());
        REQUIRE_THROWS(fut.wait());
        REQUIRE_THROWS(fut.wait_for(1ms));

        auto promise = fut.promise();
        REQUIRE_THROWS(fut.promise());

        REQUIRE(not fut.wait_for(1ms));

        SUBCASE("Valid wait")
        {
            std::thread work{
                    [promise = std::move(promise)] {
                        promise.set_value(100);
                    }};

            work.join();

            int got_value = 0;
            REQUIRE_NOTHROW(got_value = fut.get());
        }
        SUBCASE("Exception set")
        {
            std::thread work{
                    [promise = std::move(promise)] {
                        promise.set_exception(std::make_exception_ptr(std::runtime_error{"hello"}));
                    }};

            work.join();

            try {
                fut.get();
                FAIL("Exception not thrown");
            } catch (std::runtime_error&) {
                // OK.
            } catch (std::exception& e) {
                FAIL("Wrong exception thrown: " << e.what());
            }
        }
        SUBCASE("Invalid dispose")
        {
            std::thread work{[promise = std::move(promise)] {}};

            work.join();
            try {
                fut.get();
                FAIL("Exception not thrown");
            } catch (cpph::thread::future_error&) {
                // OK.
            } catch (std::exception& e) {
                FAIL("Wrong exception thrown: " << e.what());
            }
        }
    }
}
