#pragma once

#include "../detail/primitives.hxx"
#include "cpph/math/matrix.hxx"

//
#include "../detail/object_primitive_macros.hxx"

namespace cpph::math {
template <class T, int R, int C>
CPPH_REFL_DEFINE_PRIM_begin(math::matrix<T, R, C>)
{
    CPPH_REFL_primitive_type(array);
    CPPH_REFL_primitive_archive(strm, value)
    {
        *strm << reinterpret_cast<array<array<T, C>, R> const&>(value);
    }
    CPPH_REFL_primitive_restore(strm, pvalue)
    {
        *strm >> reinterpret_cast<array<array<T, C>, R>&>(*pvalue);
    }
}
CPPH_REFL_DEFINE_PRIM_end();
}  // namespace cpph::math
