#ifndef CPPHEADERS_HELPER_UTILITY_MACROS_HXX
#define CPPHEADERS_HELPER_UTILITY_MACROS_HXX
#include "helper/spdlog_macros.hxx"

#define INTERNAL_CPPH_CONCAT2(A, B) A##B
#define INTERNAL_CPPH_CONCAT(A, B)  INTERNAL_CPPH_CONCAT2(A, B)

/* generic      ***********************************************************************************/
#define CPPH_BIND(Function)                                                                            \
    [this](auto&&... args) {                                                                           \
        if constexpr (std::is_same_v<void,                                                             \
                                     decltype(this->Function(std::forward<decltype(args)>(args)...))>) \
            this->Function(std::forward<decltype(args)>(args)...);                                     \
        else                                                                                           \
            return this->Function(std::forward<decltype(args)>(args)...);                              \
    }

/* "hasher.hxx" ***********************************************************************************/
#define CPPH_UNIQUE_KEY_TYPE(TYPE)          \
    using TYPE = CPPHEADERS_NS_::basic_key< \
            INTERNAL_CPPH_CONCAT(class LABEL0##CPPH_NAMESPACE0##TYPE##II, __LINE__)>

#endif