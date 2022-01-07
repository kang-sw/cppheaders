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

#ifndef KANGSW_CPPHEADERS_ASSERT_HXX  // uses legacy include guard, to protect macros.
#define KANGSW_CPPHEADERS_ASSERT_HXX

#include <cassert>

#if defined(CPPHEADERS_IMPLEMENT_ASSERTIONS)
#    include <cstdio>
#    include <stdexcept>

#    include "third/backward.hpp"
//
#    include "third/backward.cpp.inl"
#endif

namespace cppheaders_internals {
void _assert_fails(
        char const* file, char const* func, int line, char const* expr)  //
#if defined(CPPHEADERS_IMPLEMENT_ASSERTIONS)
{
    fprintf(stderr,
            "ASSERTION FAILED: %s\n\t%s:%d\n\t  in function: %s()\n\n",
            expr, file, line, func);

    fflush(stderr);
    *((volatile int*)nullptr) = 0;  // generate segmentation fault
}
#else
        ;
#endif
}  // namespace cppheaders_internals

/**
 * Assert always
 *
 */
#define assert_(expr) \
    ((expr) ? (void)0 \
            : cppheaders_internals::_assert_fails(__FILE__, __func__, __LINE__, #expr))

#if not defined(NDEBUG)
#    define assertd_(expr) assert_(expr)
#else
#    define assertd_(expr) (void)0
#endif

#define UNIMPLEMENTED() assert_(("NOT IMPLEMENTED", 0)), throw;

#endif
