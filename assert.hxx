#ifndef KANGSW_CPPHEADERS_ASSERT_HXX  // uses legacy include guard, to protect macros.
#define KANGSW_CPPHEADERS_ASSERT_HXX

// assert always
#include "__namespace__.h"

#if defined(CPPHEADERS_IMPLEMENT_ASSERTIONS)
#  include <cstdio>
#  include <stdexcept>

#  include "third/backward.hpp"
//
#  include "third/backward.cpp.inl"
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

#define assert_(expr)                                       \
  ((void)((expr)                                            \
          || (cppheaders_internals::_assert_fails(          \
                      __FILE__, __func__, __LINE__, #expr), \
              0)))

#define UNIMPLEMENTED() assert_(("NOT IMPLEMENTED", 0)), throw;

#endif
