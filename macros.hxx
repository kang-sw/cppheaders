/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2021. Seungwoo Kang
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
 *
 * project home: https://github.com/perfkitpp
 ******************************************************************************/

#ifndef CPPHEADERS_HELPER_UTILITY_MACROS_HXX
#define CPPHEADERS_HELPER_UTILITY_MACROS_HXX
#include "helper/spdlog_macros.hxx"

#define INTERNAL_CPPH_CONCAT2(A, B) A##B
#define INTERNAL_CPPH_CONCAT(A, B)  INTERNAL_CPPH_CONCAT2(A, B)

/* generic      ***********************************************************************************/
#define CPPH_BIND(Function)                                                                            \
    [this](auto&&... args) {                                                                           \
        if constexpr (std::is_same_v<void,                                                             \
                                     decltype(this->Function(std::forward<decltype(args)>(args)...))>) \
            this->Function(std::forward<decltype(args)>(args)...);                                     \
        else                                                                                           \
            return this->Function(std::forward<decltype(args)>(args)...);                              \
    }

#define CPPH_BIND_WEAK(Function)                                                          \
    [this, weak = this->weak_from_this()](auto&&... args) {                               \
        auto self = weak.lock();                                                          \
                                                                                          \
        if constexpr (std::is_same_v<void,                                                \
                                     decltype(this->Function(                             \
                                             std::forward<decltype(args)>(args)...))>)    \
        {                                                                                 \
            if (not self)                                                                 \
                return;                                                                   \
            this->Function(std::forward<decltype(args)>(args)...);                        \
        }                                                                                 \
        else                                                                              \
        {                                                                                 \
            if (not self)                                                                 \
                return decltype(this->Function(std::forward<decltype(args)>(args)...)){}; \
            return this->Function(std::forward<decltype(args)>(args)...);                 \
        }                                                                                 \
    }

/* "hasher.hxx" ***********************************************************************************/
#define CPPH_UNIQUE_KEY_TYPE(TYPE)          \
    using TYPE = CPPHEADERS_NS_::basic_key< \
            INTERNAL_CPPH_CONCAT(class LABEL0##CPPH_NAMESPACE0##TYPE##II, __LINE__)>

/* "cleanup.hxx" **********************************************************************************/
#define CPPH_FINALLY(Callable) \
    auto INTERNAL_CPPH_CONCAT(INTERNAL_CPPH_FINALLY_, __LINE__) = CPPHEADERS_NS_::cleanup(Callable)

#endif