
/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2022. Seungwoo Kang
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

#define INTERNAL_CPPH_concat_2_(a, b) a##b
#define INTERNAL_CPPH_concat_(a, b)   INTERNAL_CPPH_concat_2_(a, b)

#define INTERNAL_CPPH_define_(ValT, Cond)                                                              \
    template <typename ValT>                                                                           \
    constexpr bool INTERNAL_CPPH_concat_(_cpph_Cond_Metadata_Line_, __LINE__) = Cond;                  \
                                                                                                       \
    template <typename ValT>                                                                           \
            struct get_object_metadata_t < ValT,                                                       \
            std::enable_if_t < INTERNAL_CPPH_concat_(_cpph_Cond_Metadata_Line_, __LINE__) < ValT >>> { \
        auto operator()() const;                                                                       \
    };                                                                                                 \
    template <typename ValT>                                                                           \
            auto get_object_metadata_t < ValT,                                                         \
            std::enable_if_t < INTERNAL_CPPH_concat_(_cpph_Cond_Metadata_Line_, __LINE__) < ValT >>> ::operator()() const

// Define new primitive type by template pattern
#define CPPH_REFL_DEF_begin(Type, ...)                \
    namespace cpph::refl {                            \
    template <typename Type>                          \
    struct get_object_metadata_t<                     \
            Type, enable_if_t<__VA_ARGS__>> {         \
        using value_type = Type;                      \
        object_metadata_t operator()() const noexcept \
        {                                             \
            static struct _manip_t : public templated_primitive_control<Type>

#define CPPH_REFL_DEF_type(TypeVal)                           \
    cpph::archive::entity_type type() const noexcept override \
    {                                                         \
        return cpph::archive::entity_type::TypeVal;           \
    }

#define CPPH_REFL_DEF_status(Param0) \
    cpph::refl::requirement_status_tag impl_status(const value_type* Param0) const noexcept override

#define CPPH_REFL_DEF_type_dynamic() \
    cpph::archive::entity_type type() const noexcept override

#define CPPH_REFL_DEF_archive(Param0, Param1) \
    void impl_archive(cpph::archive::if_writer* Param0, const value_type& Param1, cpph::refl::object_metadata_t Param2, cpph::refl::optional_property_metadata Param3) const override

#define CPPH_REFL_DEF_restore(Param0, Param1) \
    void impl_restore(cpph::archive::if_reader* Param0, value_type* Param1, cpph::refl::object_metadata_t Param2, cpph::refl::optional_property_metadata Param3) const override

#define CPPH_REFL_DEF_end()                                                                  \
    _manip;                                                                                  \
                                                                                             \
    static auto _ = object_metadata::primitive_factory::define(sizeof(value_type), &_manip); \
    return _.get();                                                                          \
    }                                                                                        \
    }                                                                                        \
    ;                                                                                        \
    }

#define CPPH_REFL_DEF_begin_single(Type) \
    cpph::refl::object_metadata_ptr      \
    initialize_object_metadata(          \
            cpph::refl::type_tag<Type>)  \
    {                                    \
        using value_type = Type;         \
        static struct _manip_t : public cpph::refl::templated_primitive_control<Type>

#define CPPH_REFL_DEF_begin_single_inline(Type) \
    inline CPPH_REFL_DEF_begin_single(Type)

#define CPPH_REFL_DEF_end_single()                                                              \
    _manip;                                                                                     \
                                                                                                \
    return cpph::refl::object_metadata::primitive_factory::define(sizeof(value_type), &_manip); \
    }