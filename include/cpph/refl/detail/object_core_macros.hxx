#ifndef CPPH_REFL_DECLARE
#define CPPH_REFL_DECLARE(ClassName)    \
    ::cpph::refl::unique_object_metadata   \
            initialize_object_metadata( \
                    cpph::refl::type_tag<ClassName>);

#define CPPH_REFL_DECLARE_c         \
    cpph::refl::unique_object_metadata \
    initialize_object_metadata();
#endif
