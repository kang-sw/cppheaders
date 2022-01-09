
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
#include "../functional.hxx"
#include "../macros.hxx"
#include "detail/object_impl.hxx"

namespace CPPHEADERS_NS_::refl {
using descriptor_generate_fn = CPPHEADERS_NS_::function<std::unique_ptr<object_descriptor>()>;
}

/**
 * All macros must be placed in global namespace!
 */
#ifndef CPPHEADERS_REFL_OBJECT_MACROS
#    define CPPHEADERS_REFL_OBJECT_MACROS

#    define CPPH_REFL_DECLARE(ClassName)                                  \
        namespace CPPHEADERS_NS_::refl {                                  \
        template <class TypeName_>                                        \
        auto get_object_descriptor()                                      \
                -> object_sfinae_t<std::is_same_v<TypeName_, ClassName>>; \
        }

#    define CPPH_REFL_DEFINE_OBJECT(ClassName, FactoryName)

#    define CPPH_REFL_DEFINE_TUPLE(ClassName, FactoryName)

#    define CPPH_prop(VarName) \
        &std::remove_pointer_t<decltype(INTERNAL_CPPH_CLASSPTR)>::VarName

#    define CPPH_prop_0(VarName) \
        property(CPPH_prop(VarName))

#    define CPPH_prop_1(PropNameStr, VarName) \
        property(PropNameStr, CPPH_prop(VarName))

#    define CPPH_prop_2(VarName) CPPH_prop_1(#    VarName, VarName)

#    define INTERNAL_CPPH_REFL_GENERATOR_NAME(ClassName) \
        INTERNAL_CPPH_CONCAT(INTERNAL_CPPH_CONCAT(INTERNAL_cpph_refl_generator, ClassName), __LINE__)

#    define INTERNAL_CPPH_REFL_CLASS_NAME(ClassName) \
        INTERNAL_CPPH_CONCAT(INTERNAL_CPPH_CONCAT(INTERNAL_cpph_refl_class, ClassName), __LINE__)

#    define INTERNAL_CPPH_DEFINE_IMPL(ClassName, FactoryName, Registration)                               \
        using INTERNAL_CPPH_REFL_CLASS_NAME(ClassName) = ClassName;                                       \
        extern CPPHEADERS_NS_::refl::descriptor_generate_fn INTERNAL_CPPH_REFL_GENERATOR_NAME(ClassName); \
                                                                                                          \
        namespace CPPHEADERS_NS_::refl {                                                                  \
                                                                                                          \
        template <class TypeName_>                                                                        \
        auto get_object_descriptor()                                                                      \
                -> object_sfinae_t<std::is_same_v<TypeName_, INTERNAL_CPPH_REFL_CLASS_NAME(ClassName)>>   \
        {                                                                                                 \
            static auto instance = INTERNAL_CPPH_REFL_GENERATOR_NAME(ClassName)();                        \
            return &*instance;                                                                            \
        }                                                                                                 \
        }                                                                                                 \
                                                                                                          \
        CPPHEADERS_NS_::refl::descriptor_generate_fn INTERNAL_CPPH_REFL_GENERATOR_NAME(ClassName)         \
                = [ INTERNAL_CPPH_CLASSPTR = (INTERNAL_CPPH_REFL_CLASS_NAME(ClassName)*)nullptr,          \
                    FactoryName            = Registration<INTERNAL_CPPH_REFL_CLASS_NAME(ClassName)>() ]

#endif

/*
namespace cpph::refl {
template <class T>
auto get_object_descriptor()
        -> cpph::refl::object_sfinae_t<std::is_same_v<T, test_object>>
{
    static auto instance = [] {
        return object_descriptor::object_factory{}
                .start(sizeof(test_object))
                .add_property("a", {offsetof(test_object, a), default_object_descriptor_fn<int>(), [](void*) {}})
                .add_property("b", {offsetof(test_object, b), default_object_descriptor_fn<int>(), [](void*) {}})
                .add_property("c", {offsetof(test_object, c), default_object_descriptor_fn<int>(), [](void*) {}})
                .create();
    }();

return &instance;
}
 */