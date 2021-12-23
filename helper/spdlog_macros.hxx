#ifndef KANGSW_CPPHEADERS_SPDLOG_HELPER_MACROS_HXX
#define KANGSW_CPPHEADERS_SPDLOG_HELPER_MACROS_HXX
#if __has_include("spdlog/spdlog.h")

// #define CPPH_LOGGER (your_logger)

#    define CPPH_TRACE(...)    SPDLOG_LOGGER_TRACE(CPPH_LOGGER(), __VA_ARGS__)
#    define CPPH_DEBUG(...)    SPDLOG_LOGGER_DEBUG(CPPH_LOGGER(), __VA_ARGS__)
#    define CPPH_INFO(...)     SPDLOG_LOGGER_INFO(CPPH_LOGGER(), __VA_ARGS__)
#    define CPPH_WARN(...)     SPDLOG_LOGGER_WARN(CPPH_LOGGER(), __VA_ARGS__)
#    define CPPH_ERROR(...)    SPDLOG_LOGGER_ERROR(CPPH_LOGGER(), __VA_ARGS__)
#    define CPPH_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(CPPH_LOGGER(), __VA_ARGS__)

#    define CPPH_LOG_TRACE    (CPPH_LOGGER()->should_log(spdlog::level::info))
#    define CPPH_LOG_DEBUG    (CPPH_LOGGER()->should_log(spdlog::level::debug))
#    define CPPH_LOG_INFO     (CPPH_LOGGER()->should_log(spdlog::level::info))
#    define CPPH_LOG_WARN     (CPPH_LOGGER()->should_log(spdlog::level::warn))
#    define CPPH_LOG_ERROR    (CPPH_LOGGER()->should_log(spdlog::level::error))
#    define CPPH_LOG_CRITICAL (CPPH_LOGGER()->should_log(spdlog::level::critical))

#endif
#endif
