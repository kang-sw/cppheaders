#include "cpph/refl/object.hxx"

struct test_bin {
    int x, y, z;
};

CPPH_REFL_DEFINE_PRIM_binary(test_bin);
