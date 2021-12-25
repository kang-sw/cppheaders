/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2021.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

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
