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

#ifndef KANGSW_CPPHEADERS_ARCHIVING_HELPER_MACROS_HXX
#define KANGSW_CPPHEADERS_ARCHIVING_HELPER_MACROS_HXX
#include "__macro_utility.hxx"

#define INTERNAL_CPPHDRS_CONCAT2(A, B) A##B
#define INTERNAL_CPPHDRS_CONCAT(A, B)  INTERNAL_CPPHDRS_CONCAT2(A, B)

#define INTERNAL_CPPHDRS_ARCHIVING_BRK_TOKENS(STR)                                                            \
    static constexpr char INTERNAL_CPPHDRS_CONCAT(KEYSTR_, __LINE__)[] = STR;                                 \
    static constexpr auto INTERNAL_CPPHDRS_CONCAT(KEYSTR_VIEW_, __LINE__)                                     \
            = CPPHEADERS_NS_::archiving::detail::break_VA_ARGS<INTERNAL_CPPHDRS_CONCAT(KEYSTR_, __LINE__)>(); \
    static const inline auto INTERNAL_CPPHDRS_CONCAT(KEYARRAY_, __LINE__)                                     \
            = CPPHEADERS_NS_::archiving::detail::views_to_strings(INTERNAL_CPPHDRS_CONCAT(KEYSTR_VIEW_, __LINE__));

#if __has_include("nlohmann/json.hpp")
#    include <nlohmann/json.hpp>

#    define INTERNAL_CPPHDRS_NLOHMANN_JSON_REGISTER(CLASSNAME, ...) \
        void                                                        \
        from_json(nlohmann::json const& r)                          \
        {                                                           \
            namespace _ns = CPPHEADERS_NS_::archiving::detail;      \
                                                                    \
            _ns::visit_with_key(                                    \
                    INTERNAL_CPPHDRS_CONCAT(KEYARRAY_, __LINE__),   \
                    _ns::from_json_visitor(r),                      \
                    __VA_ARGS__);                                   \
        }                                                           \
                                                                    \
        friend void                                                 \
        from_json(nlohmann::json const& r, CLASSNAME& to)           \
        {                                                           \
            to.from_json(r);                                        \
        }                                                           \
                                                                    \
        void                                                        \
        to_json(nlohmann::json& r) const noexcept                   \
        {                                                           \
            namespace _ns = CPPHEADERS_NS_::archiving::detail;      \
                                                                    \
            CPPHEADERS_NS_::archiving::detail::visit_with_key(      \
                    INTERNAL_CPPHDRS_CONCAT(KEYARRAY_, __LINE__),   \
                    _ns::to_json_visitor(r),                        \
                    __VA_ARGS__);                                   \
        }                                                           \
                                                                    \
        friend void                                                 \
        to_json(nlohmann::json& r, CLASSNAME const& to) noexcept    \
        {                                                           \
            to.to_json(r);                                          \
        }

#else
#    define INTERNAL_CPPHDRS_NLOHMANN_JSON_REGISTER()
#endif
#define CPPHEADERS_DEFINE_NLOHMANN_JSON_ARCHIVER(CLASSNAME, ...) \
    INTERNAL_CPPHDRS_ARCHIVING_BRK_TOKENS(#__VA_ARGS__);         \
    INTERNAL_CPPHDRS_NLOHMANN_JSON_REGISTER(CLASSNAME, __VA_ARGS__);
#endif
