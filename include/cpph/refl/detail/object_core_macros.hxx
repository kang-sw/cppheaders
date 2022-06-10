#ifndef CPPH_REFL_DECLARE
#define CPPH_REFL_DECLARE(ClassName)    \
    ::cpph::refl::object_metadata_ptr   \
            initialize_object_metadata( \
                    cpph::refl::type_tag<ClassName>);

#define CPPH_REFL_DECLARE_c         \
    cpph::refl::object_metadata_ptr \
    initialize_object_metadata();
#endif
