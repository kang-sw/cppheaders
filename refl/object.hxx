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

#pragma once
#include "../helper/macro_utility.hxx"
#include "detail/primitives.hxx"
#include "object_core.hxx"

#ifndef CPPHEADERS_REFL_OBJECT_MACROS
#    define CPPHEADERS_REFL_OBJECT_MACROS

#    define CPPH_REFL_DEFINE_OBJECT_c(ClassName, RESERVED, ...) \
        INTERNAL_CPPH_REFL_EMBED_DEFINE_IMPL(ClassName::, RESERVED, define_object, INTERNAL_CPPH_REFL_ITERATE_OJBECT_VAR, __VA_ARGS__)

#    define CPPH_REFL_DEFINE_OBJECT_inline(RESERVED, ...) \
        INTERNAL_CPPH_REFL_EMBED_DEFINE_IMPL(, RESERVED, define_object, INTERNAL_CPPH_REFL_ITERATE_OJBECT_VAR, __VA_ARGS__)

#    define CPPH_REFL_DEFINE_TUPLE_c(ClassName, RESERVED, ...) \
        INTERNAL_CPPH_REFL_EMBED_DEFINE_IMPL(ClassName::, RESERVED, define_tuple, INTERNAL_CPPH_REFL_ITERATE_TUPLE_VAR, __VA_ARGS__)

#    define CPPH_REFL_DEFINE_TUPLE_inline(RESERVED, ...) \
        INTERNAL_CPPH_REFL_EMBED_DEFINE_IMPL(, RESERVED, define_tuple, INTERNAL_CPPH_REFL_ITERATE_TUPLE_VAR, __VA_ARGS__)

#    define CPPH_REFL_DEFINE_OBJECT(ClassName, RESERVED, ...) \
        INTERNAL_CPPH_REFL_DEFINE_IMPL(ClassName, RESERVED, define_object, INTERNAL_CPPH_REFL_ITERATE_OJBECT_VAR, __VA_ARGS__)

#    define CPPH_REFL_DEFINE_TUPLE(ClassName, RESERVED, ...) \
        INTERNAL_CPPH_REFL_DEFINE_IMPL(ClassName, RESERVED, define_tuple, INTERNAL_CPPH_REFL_ITERATE_TUPLE_VAR, __VA_ARGS__)

#    define INTERNAL_CPPH_REFL_EMBED_DEFINE_IMPL(Qualify, RESERVED, FactoryType, Iterator, ...) \
        CPPHEADERS_NS_::refl::object_metadata_ptr                                               \
                Qualify                                                                         \
                initialize_object_metadata()                                                    \
        {                                                                                       \
            INTERNAL_CPPH_reserved RESERVED;                                                    \
            using ClassName             = std::remove_pointer_t<decltype(this)>;                \
            using self_t                = ClassName;                                            \
            auto _cpph_internal_factory = CPPHEADERS_NS_::refl::FactoryType<ClassName>();       \
                                                                                                \
            CPPH_FOR_EACH(Iterator, __VA_ARGS__);                                               \
                                                                                                \
            return _cpph_internal_factory.create();                                             \
        }

#    define INTERNAL_CPPH_REFL_ITERATE_OJBECT_VAR_3(VarName, ...) \
        _cpph_internal_factory._property_3(&self_t::VarName, #VarName, ##__VA_ARGS__)

#    define INTERNAL_CPPH_REFL_ITERATE_OJBECT_VAR(Param) \
        INTERNAL_CPPH_REFL_ITERATE_OJBECT_VAR_3 Param

#    define INTERNAL_CPPH_REFL_ITERATE_TUPLE_VAR(VarName) \
        _cpph_internal_factory.property(&self_t::VarName)

#    define INTERNAL_CPPH_reserved()

#    define INTERNAL_CPPH_REFL_DEFINE_IMPL(ClassName, RESERVED, FactoryType, Iterator, ...) \
        CPPHEADERS_NS_::refl::object_metadata_ptr                                           \
        initialize_object_metadata(                                                         \
                CPPHEADERS_NS_::refl::type_tag<ClassName>)                                  \
        {                                                                                   \
            INTERNAL_CPPH_reserved RESERVED;                                                \
            using self_t                = ClassName;                                        \
            auto _cpph_internal_factory = CPPHEADERS_NS_::refl::FactoryType<ClassName>();   \
                                                                                            \
            CPPH_FOR_EACH(Iterator, __VA_ARGS__);                                           \
                                                                                            \
            return _cpph_internal_factory.create();                                         \
        }

#endif
