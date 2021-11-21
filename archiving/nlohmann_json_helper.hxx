#ifndef KANGSW_CPPHEADERS_ARCHIVING_HELPER_MACROS_HXX
#define KANGSW_CPPHEADERS_ARCHIVING_HELPER_MACROS_HXX
#include "__macro_utility.hxx"

#define INTERNAL_CPPHDRS_ARCHIVING_BRK_TOKENS(STR)                                 \
  static constexpr char KEYSTR_##__LINE__[] = STR;                                 \
  static constexpr auto KEYSTR_VIEW_##__LINE__                                     \
          = CPPHEADERS_NS_::archiving::detail::break_VA_ARGS<KEYSTR_##__LINE__>(); \
  static const inline auto KEYARRAY_##__LINE__                                     \
          = CPPHEADERS_NS_::archiving::detail::views_to_strings(KEYSTR_VIEW_##__LINE__);

#if __has_include("nlohmann/json.hpp")
#  include <nlohmann/json.hpp>

#  define INTERNAL_CPPHDRS_NLOHMANN_JSON_REGISTER(CLASSNAME, ...) \
    void                                                          \
    from_json(nlohmann::json const& r) {                          \
      namespace _ns = CPPHEADERS_NS_::archiving::detail;          \
                                                                  \
      _ns::visit_with_key(                                        \
              KEYARRAY_##__LINE__,                                \
              _ns::from_json_visitor(r),                          \
              __VA_ARGS__);                                       \
    }                                                             \
                                                                  \
    friend void                                                   \
    from_json(nlohmann::json const& r, CLASSNAME& to) {           \
      to.from_json(r);                                            \
    }                                                             \
                                                                  \
    void                                                          \
    to_json(nlohmann::json& r) const noexcept {                   \
      namespace _ns = CPPHEADERS_NS_::archiving::detail;          \
                                                                  \
      CPPHEADERS_NS_::archiving::detail::visit_with_key(          \
              KEYARRAY_##__LINE__,                                \
              _ns::to_json_visitor(r),                            \
              __VA_ARGS__);                                       \
    }                                                             \
                                                                  \
    friend void                                                   \
    to_json(nlohmann::json& r, CLASSNAME const& to) noexcept {    \
      to.to_json(r);                                              \
    }

#else
#  define INTERNAL_CPPHDRS_NLOHMANN_JSON_REGISTER()
#endif
#define CPPHEADERS_DEFINE_NLOHMANN_JSON_ARCHIVER(CLASSNAME, ...) \
  INTERNAL_CPPHDRS_ARCHIVING_BRK_TOKENS(#__VA_ARGS__);           \
  INTERNAL_CPPHDRS_NLOHMANN_JSON_REGISTER(CLASSNAME, __VA_ARGS__);
#endif
