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
#include "macro_utility.hxx"

#if __has_include("nlohmann/json.hpp")
#    include <nlohmann/json.hpp>

#    define INTERNAL_CPPHDRS_NLOHMANN_JSON_REGISTER(CLASSNAME, ...) \
        void                                                        \
        from_json(nlohmann::json const& r)                          \
        {                                                           \
            namespace _ns = CPPHEADERS_NS_::macro_utils;            \
                                                                    \
            _ns::visit_with_key(                                    \
                    INTERNAL_CPPH_BRK_TOKENS_ACCESS_ARRAY(),        \
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
            namespace _ns = CPPHEADERS_NS_::macro_utils;            \
                                                                    \
            r = nlohmann::json::object();                           \
            CPPHEADERS_NS_::macro_utils::visit_with_key(            \
                    INTERNAL_CPPH_BRK_TOKENS_ACCESS_ARRAY(),        \
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
    INTERNAL_CPPH_ARCHIVING_BRK_TOKENS(#__VA_ARGS__);            \
    INTERNAL_CPPHDRS_NLOHMANN_JSON_REGISTER(CLASSNAME, __VA_ARGS__);
#endif
