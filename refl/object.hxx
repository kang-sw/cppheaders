// MIT License
//
// Copyright (c) 2022. Seungwoo Kang
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
#include "../helper/__macro_utility.hxx"
#include "detail/primitives.hxx"
#include "object_core.hxx"

#ifndef CPPHEADERS_REFL_OBJECT_MACROS
#    define CPPHEADERS_REFL_OBJECT_MACROS

#    define CPPH_REFL_DEFINE_OBJECT_c(ClassName, ...)      INTERNAL_CPPH_REFL_EMBED_DEFINE(ClassName::, ClassName, define_object, (key, this, &value), __VA_ARGS__)
#    define CPPH_REFL_DEFINE_OBJECT_inline(ClassName, ...) INTERNAL_CPPH_REFL_EMBED_DEFINE(, ClassName, define_object, (key, this, &value), __VA_ARGS__)
#    define CPPH_REFL_DEFINE_TUPLE_c(ClassName, ...)       INTERNAL_CPPH_REFL_EMBED_DEFINE(ClassName::, ClassName, define_tuple, (this, &value), __VA_ARGS__)
#    define CPPH_REFL_DEFINE_TUPLE_inline(ClassName, ...)  INTERNAL_CPPH_REFL_EMBED_DEFINE(, ClassName, define_tuple, (this, &value), __VA_ARGS__)

#    define CPPH_REFL_DEFINE_OBJECT(ClassName)          INTERNAL_CPPH_DEFINE_IMPL(ClassName, factory, CPPHEADERS_NS_::refl::define_object)
#    define CPPH_REFL_DEFINE_TUPLE(ClassName)           INTERNAL_CPPH_DEFINE_IMPL(ClassName, factory, CPPHEADERS_NS_::refl::define_tuple)
#    define CPPH_PROP_TUPLE(VarName, ...)               property(INTERNAL_CPPH_REFL_RETRIEVE_MEMPTR(VarName), ##__VA_ARGS__)
#    define CPPH_PROP_OBJECT(PropNameStr, VarName, ...) property(PropNameStr, INTERNAL_CPPH_REFL_RETRIEVE_MEMPTR(VarName), ##__VA_ARGS__)
#    define CPPH_PROP_OBJECT_AUTOKEY(VarName, ...)      CPPH_PROP_OBJECT(#    VarName, VarName, ##__VA_ARGS__)

#    define INTERNAL_CPPH_REFL_EMBED_DEFINE(Qualify, ClassName, FactoryType, ParamType, ...) \
        CPPHEADERS_NS_::refl::object_descriptor_ptr                                          \
                Qualify                                                                      \
                initialize_object_descriptor()                                               \
        {                                                                                    \
            INTERNAL_CPPH_ARCHIVING_BRK_TOKENS_0(#__VA_ARGS__)                               \
            auto _cpph_internal_factory = CPPHEADERS_NS_::refl::FactoryType<ClassName>();    \
                                                                                             \
            CPPHEADERS_NS_::macro_utils::visit_with_key(                                     \
                    INTERNAL_CPPH_BRK_TOKENS_ACCESS_VIEW(),                                  \
                    [&](std::string_view key, auto& value) {                                 \
                        _cpph_internal_factory.property ParamType;                           \
                    },                                                                       \
                    __VA_ARGS__);                                                            \
                                                                                             \
            return _cpph_internal_factory.create();                                          \
        }

#    define INTERNAL_CPPH_REFL_RETRIEVE_MEMPTR(VarName) \
        &std::remove_pointer_t<decltype(INTERNAL_CPPH_CLASSPTR)>::VarName

#    define INTERNAL_CPPH_REFL_GENERATOR_NAME() \
        INTERNAL_CPPH_CONCAT(INTERNAL_cpph_refl_generator_, __LINE__)

#    define INTERNAL_CPPH_REFL_CLASS_NAME() \
        INTERNAL_CPPH_CONCAT(INTERNAL_cpph_refl_class_, __LINE__)

#    define INTERNAL_CPPH_DEFINE_IMPL(ClassName, FactoryName, Registration)                      \
                                                                                                 \
        using INTERNAL_CPPH_REFL_CLASS_NAME() = ClassName;                                       \
        extern CPPHEADERS_NS_::refl::descriptor_generate_fn INTERNAL_CPPH_REFL_GENERATOR_NAME(); \
                                                                                                 \
        namespace CPPHEADERS_NS_::refl {                                                         \
                                                                                                 \
        template <class TypeName_>                                                               \
        auto get_object_descriptor()                                                             \
                -> object_sfinae_t<std::is_same_v<TypeName_, INTERNAL_CPPH_REFL_CLASS_NAME()>>   \
        {                                                                                        \
            static auto instance = INTERNAL_CPPH_REFL_GENERATOR_NAME()();                        \
            return &*instance;                                                                   \
        }                                                                                        \
        }                                                                                        \
                                                                                                 \
        CPPHEADERS_NS_::refl::descriptor_generate_fn INTERNAL_CPPH_REFL_GENERATOR_NAME()         \
                = [ INTERNAL_CPPH_CLASSPTR = (INTERNAL_CPPH_REFL_CLASS_NAME()*)nullptr,          \
                    FactoryName            = (Registration<INTERNAL_CPPH_REFL_CLASS_NAME()>()) ]

#endif
