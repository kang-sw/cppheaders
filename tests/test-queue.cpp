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

#include <vector>

#include <memory/ring_allocator.hxx>

#include "catch.hpp"

TEST_CASE("overall", "[queue_allocator]")
{
    enum {
        BUFLEN = 1024,
        NMAX = BUFLEN / 8 / 2
    };

    cpph::ring_allocator buffer{BUFLEN};
    REQUIRE(buffer.capacity() == BUFLEN);
    REQUIRE_THROWS(buffer.allocate(BUFLEN));

    {
        void* ptr;
        REQUIRE_NOTHROW(ptr = buffer.allocate(BUFLEN - 8));
        buffer.deallocate(ptr);
        REQUIRE(buffer.empty());
    }

    // Allocate full times (max)
    for (int i = 0; i < NMAX; ++i) {
        REQUIRE_NOTHROW(*(int*)buffer.allocate(4) = i + 1);
    }

    // Must be full
    REQUIRE_THROWS(buffer.allocate(0));

    // Deallocate half
    for (int i = 0; i < NMAX; ++i) {
        REQUIRE(*(int*)buffer.front() == i + 1);
        buffer.deallocate(buffer.front());
    }

    REQUIRE(buffer.empty());
}

TEST_CASE("buffer deferred dealloc", "[queue_allocator]")
{
}
