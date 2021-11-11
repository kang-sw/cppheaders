#pragma once
#include <cassert>

// assert always
#include "__namespace__.h"

#if defined(CPPHEADERS_IMPLEMENT_ASSERTIONS)
#include <cstdio>
#include <stdexcept>
#endif

namespace cppheaders_internals {
void _assert_fails(
        char const* file, char const* func, int line, char const* expr)  //
#if defined(CPPHEADERS_IMPLEMENT_ASSERTIONS)
{
  char buf[1024];
  snprintf(buf, sizeof buf, "%s:%d (%s): (%s) == false", file, line, func, expr);
  fprintf(stderr, "ASSERTION FAILED: %s", buf);

  fflush(stderr);
  *((volatile int*)nullptr) = 0;  // generate segmentation fault
}
#else
        ;
#endif
}  // namespace cppheaders_internals

#ifdef NDEBUG
#undef assert
#define assert(expr)                                 \
  (!(expr)                                           \
   && (cppheaders_internals::_assert_fails(          \
               __FILE__, __LINE__, __func__, #expr), \
       0))
#endif
