#undef CPPHEADERS_NS_

#if __has_include("../__cppheaders_ns__.h")
// Content of __cppheaders_ns__.h should be following:
// #define CPPHEADERS_NS_ YOUR_NAMESPACE
#include "../__cppheaders_ns__.h"
#else
// for legacy compatibility
#define CPPHEADERS_NS_ KANGSW_TEMPLATE_NAMESPACE
#endif
