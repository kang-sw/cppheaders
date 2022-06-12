#pragma once
#include "cpph/refl/object.hxx"
#include "cpph/utility/hasher.hxx"

CPPH_REFL_DEFINE_PRIM_T_begin(ValueType, is_template_instance_of<ValueType, basic_key>::value)
{
    CPPH_REFL_primitive_type(integer);

    CPPH_REFL_primitive_archive(strm, data)
    {
        *strm << data.value;
    }

    CPPH_REFL_primitive_restore(strm, p_data)
    {
        *strm >> *p_data;
    }
}
CPPH_REFL_DEFINE_PRIM_T_end();
