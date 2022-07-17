/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2021-2022. Seungwoo Kang
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
#include "spdlog_macros.hxx"

#define INTERNAL_CPPH_CONCAT2(A, B) A##B
#define INTERNAL_CPPH_CONCAT(A, B)  INTERNAL_CPPH_CONCAT2(A, B)

#define INTERNAL_CPPH_STRINGIFY2(A) #A
#define INTERNAL_CPPH_STRINGIFY(A)  INTERNAL_CPPH_STRINGIFY2(A)

/* temporary variable name ************************************************************************/
#define CPPH_TMPVAR auto INTERNAL_CPPH_CONCAT(_internal_cpph_tmpvar_, __LINE__)
#define CPPH_TMP    INTERNAL_CPPH_CONCAT(_internal_cpph_tmpvar_, __LINE__)

/* generic      ***********************************************************************************/
#define CPPH_BIND(Function)                                                                     \
    [this](auto&&... args) {                                                                    \
        if constexpr (std::is_void_v<                                                           \
                              decltype(this->Function(std::forward<decltype(args)>(args)...))>) \
            this->Function(std::forward<decltype(args)>(args)...);                              \
        else                                                                                    \
            return this->Function(std::forward<decltype(args)>(args)...);                       \
    }

#define CPPH_BIND_WEAK(Function)                                                          \
    [this, weak = this->weak_from_this()](auto&&... args) {                               \
        auto self = weak.lock();                                                          \
                                                                                          \
        if constexpr (std::is_same_v<void,                                                \
                                     decltype(this->Function(                             \
                                             std::forward<decltype(args)>(args)...))>) {  \
            if (not self)                                                                 \
                return;                                                                   \
            this->Function(std::forward<decltype(args)>(args)...);                        \
        } else {                                                                          \
            if (not self)                                                                 \
                return decltype(this->Function(std::forward<decltype(args)>(args)...)){}; \
            return this->Function(std::forward<decltype(args)>(args)...);                 \
        }                                                                                 \
    }

/* "hasher.hxx" ***********************************************************************************/
#define CPPH_UNIQUE_KEY_TYPE(TYPE) \
    using TYPE = cpph::basic_key<  \
            INTERNAL_CPPH_CONCAT(class LABEL0##CPPH_NAMESPACE0##TYPE##II, __LINE__)>

/* "cleanup.hxx" **********************************************************************************/
#define CPPH_CLEANUP(...) \
    [[maybe_unused]] auto INTERNAL_CPPH_CONCAT(INTERNAL_CPPH_FINALLY_, __LINE__) = cpph::cleanup(__VA_ARGS__)

#define CPPH_FINALLY(...) \
    [[maybe_unused]] auto INTERNAL_CPPH_CONCAT(INTERNAL_CPPH_FINALLY_, __LINE__) = cpph::cleanup([&] { __VA_ARGS__; })

/* "template_utils.hxx" ***************************************************************************/
#define CPPH_SFINAE_EXPR(Name, TParam, ...)                                    \
    template <typename TParam, class = void>                                   \
    struct Name : std::false_type {                                            \
    };                                                                         \
    template <typename TParam>                                                 \
    struct Name<TParam, std::void_t<decltype(__VA_ARGS__)>> : std::true_type { \
    };                                                                         \
                                                                               \
    template <typename TParam>                                                 \
    constexpr bool INTERNAL_CPPH_CONCAT(Name, _v) = Name<TParam>::value;

#define CPPH_REQUIRE(TemplateCond) class = std::enable_if_t<(TemplateCond)>

/* alloca *****************************************************************************************/
namespace cpph::_detail {
static inline int& _tmp_int() noexcept
{
    static thread_local int value = 0;
    return value;
}
}  // namespace cpph::_detail

// TODO: Implement corresponding logic ...
#define CPPH_ALLOCA_LIST(TypeName, VarName) cpph::_detail::alloca_fwd_list<TypeName> VarName
#define CPPH_ALLOCA_LIST_EMPLACE(S, ...)    S._emplace_with(alloca(decltype(S)::node_size), __VA_ARGS__)

#define CPPH_ALLOCA_ARR(Type, Size) (                             \
        cpph::_detail::_tmp_int() = (Size),                       \
        cpph::create_temporary_array<Type>(                       \
                alloca(sizeof(Type) * cpph::_detail::_tmp_int()), \
                cpph::_detail::_tmp_int()))

#define CPPH_ALLOCA_CSTR(sview)                         \
    (std::data(sview)[std::size(sview)] == 0            \
             ? std::data(sview)                         \
             : ([](char* p, void const* s, size_t n) {  \
                   return memcpy(p, s, n), p[n] = 0, p; \
               }((char*)alloca(std::size(sview) + 1), std::data(sview), std::size(sview))))
#endif
