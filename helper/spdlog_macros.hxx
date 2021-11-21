#ifndef KANGSW_CPPHEADERS_SPDLOG_HELPER_MACROS_HXX
#define KANGSW_CPPHEADERS_SPDLOG_HELPER_MACROS_HXX
#if __has_include("spdlog/spdlog.h")

// #define CPPH_LOGGER (your_logger)

#  define L_TRACE(...)    SPDLOG_TRACE(CPPH_LOGGER, __VA_ARGS__)
#  define L_DEBUG(...)    SPDLOG_DEBUG(CPPH_LOGGER, __VA_ARGS__)
#  define L_INFO(...)     SPDLOG_INFO(CPPH_LOGGER, __VA_ARGS__)
#  define L_WARN(...)     SPDLOG_WARN(CPPH_LOGGER, __VA_ARGS__)
#  define L_CRITICAL(...) SPDLOG_CRITICAL(CPPH_LOGGER, __VA_ARGS__)

#endif
#endif
