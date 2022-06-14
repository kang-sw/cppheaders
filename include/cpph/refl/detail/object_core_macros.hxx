#ifndef CPPH_REFL_DECLARE

/**
 * Add refl metadata object declaration outside of class.
 *
 * This must be placed in the same namespace with given clasw!
 */
#define CPPH_REFL_DECLARE(ClassName)     \
    ::cpph::refl::unique_object_metadata \
            initialize_object_metadata(  \
                    cpph::refl::type_tag<ClassName>);

/**
 * Add refl metadata object declaration inside of class.
 */
#define CPPH_REFL_DECLARE_em(ClassName)          \
    friend ::cpph::refl::unique_object_metadata \
            initialize_object_metadata(         \
                    cpph::refl::type_tag<ClassName>);

/**
 * Add member function version of object declaration
 */
#define CPPH_REFL_DECLARE_c            \
    cpph::refl::unique_object_metadata \
    initialize_object_metadata();
#endif
