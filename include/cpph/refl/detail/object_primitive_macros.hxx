
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
#define IIT_CPPH_REFLD_begin(Type, ...)               \
    namespace cpph::refl {                            \
    template <typename Type>                          \
    struct get_object_metadata_t<                     \
            Type, enable_if_t<__VA_ARGS__>> {         \
        using value_type = Type;                      \
        object_metadata_t operator()() const noexcept \
        {                                             \
            static struct _manip_t : public templated_primitive_control<Type>

#define CPPH_REFL_primitive_type(TypeVal)                     \
    cpph::archive::entity_type type() const noexcept override \
    {                                                         \
        return cpph::archive::entity_type::TypeVal;           \
    }

#define CPPH_REFL_primitive_status(ValuePtrParamName) \
    cpph::refl::requirement_status_tag impl_status(const value_type* ValuePtrParamName) const noexcept override

#define CPPH_REFL_primitive_type_dynamic() \
    cpph::archive::entity_type type() const noexcept override

#define CPPH_REFL_primitive_archive(StrmParamName, ValueParamName) \
    void impl_archive(cpph::archive::if_writer* StrmParamName, const value_type& ValueParamName, cpph::refl::object_metadata_t Param2, cpph::refl::optional_property_metadata Param3) const override

#define CPPH_REFL_primitive_restore(StrmParamName, ValuePtrParamName) \
    void impl_restore(cpph::archive::if_reader* StrmParamName, value_type* ValuePtrParamName, cpph::refl::object_metadata_t Param2, cpph::refl::optional_property_metadata Param3) const override

#define IIT_CPPH_REFLD_end()                                                                 \
    _manip;                                                                                  \
                                                                                             \
    static auto _ = object_metadata::primitive_factory::define(sizeof(value_type), &_manip); \
    return _.get();                                                                          \
    }                                                                                        \
    }                                                                                        \
    ;                                                                                        \
    }

#define IIT_CPPH_REFLD_begin_single(...) \
    cpph::refl::unique_object_metadata    \
    initialize_object_metadata(           \
            cpph::refl::type_tag<__VA_ARGS__>)   \
    {                                     \
        using value_type = __VA_ARGS__;          \
        static struct _manip_t : public cpph::refl::templated_primitive_control<__VA_ARGS__>

#define IIT_CPPH_REFLD_begin_single_inline(Type) \
    inline IIT_CPPH_REFLD_begin_single(Type)

#define IIT_CPPH_REFLD_end_single()                                                             \
    _manip;                                                                                     \
                                                                                                \
    return cpph::refl::object_metadata::primitive_factory::define(sizeof(value_type), &_manip); \
    }

#define CPPH_REFL_DEFINE_PRIM_T_begin IIT_CPPH_REFLD_begin
#define CPPH_REFL_DEFINE_PRIM_T_end   IIT_CPPH_REFLD_end
#define CPPH_REFL_DEFINE_PRIM_begin   IIT_CPPH_REFLD_begin_single
#define CPPH_REFL_DEFINE_PRIM_end     IIT_CPPH_REFLD_end_single

#define CPPH_REFL_DEFINE_PRIM_binary(Struct)                                                  \
    IIT_CPPH_REFLD_begin_single(Struct)                                                       \
    {                                                                                         \
        static_assert(std::is_trivial_v<Struct>, "Only trivial struct can be binary object"); \
        CPPH_REFL_primitive_type(binary);                                                     \
                                                                                              \
        CPPH_REFL_primitive_archive(strm, value)                                              \
        {                                                                                     \
            strm->binary_push(sizeof value);                                                  \
            strm->binary_write_some({&value, 1});                                             \
            strm->binary_pop();                                                               \
        }                                                                                     \
                                                                                              \
        CPPH_REFL_primitive_restore(strm, p_value)                                            \
        {                                                                                     \
            auto n = strm->begin_binary();                                                    \
            n = std::min(n, sizeof *p_value);                                                 \
            strm->binary_read_some({p_value, 1});                                             \
            strm->end_binary();                                                               \
        }                                                                                     \
    }                                                                                         \
    IIT_CPPH_REFLD_end_single()
