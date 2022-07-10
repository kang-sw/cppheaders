#pragma once
#include <cpph/std/memory>
#include <cpph/std/string>

#include <cpph/utility/hasher.hxx>

namespace cpph::archive {
class if_writer;
class if_reader;
}  // namespace cpph::archive

namespace cpph {
using namespace cpph;

enum class localization_result {
    okay,
    invalid_file_path,
    invalid_content,
    already_loaded,
    locale_not_loaded,
};

localization_result load_localization_lut(string_view key, archive::if_reader*);
localization_result load_localization_lut(string_view key, string const& path);
localization_result select_locale(string_view key);
localization_result dump_localization_lut(archive::if_writer*);
localization_result dump_localization_lut(string_view path);

}  // namespace cpph

namespace cpph::_detail {
struct loca_static_context;

loca_static_context* loca_create_static_context(uint64_t hash, char const* ref_text, char const* label) noexcept;
string const& loca_lookup(loca_static_context*) noexcept;
}  // namespace cpph::_detail

#define INTERNALCPPH_LOCTEXT_FULL(HashStr, RefText, Label)                                \
    ([]() -> std::string const& {                                                            \
        static constexpr auto hash = cpph::hasher::fnv1a_64(HashStr);                        \
        static auto ctx = cpph::_detail::loca_create_static_context(hash, RefText, Label); \
        return cpph::_detail::loca_lookup(ctx);                                            \
    }())

#define CPPH_KEYTEXT(Label, RefText) INTERNALCPPH_LOCTEXT_FULL(#Label, RefText, #Label)
#define CPPH_KEYWORD(Label)          INTERNALCPPH_LOCTEXT_FULL(#Label, #Label, #Label)
#define CPPH_LOCTEXT(RefText)        INTERNALCPPH_LOCTEXT_FULL(RefText, RefText, nullptr)
#define CPPH_LOCWORD(RefText)        INTERNALCPPH_LOCTEXT_FULL(RefText, RefText, RefText)

#define CPPH_C_KEYTEXT(Label, RefText) INTERNALCPPH_LOCTEXT_FULL(#Label, RefText, #Label).c_str()
#define CPPH_C_KEYWORD(Label)          INTERNALCPPH_LOCTEXT_FULL(#Label, #Label, #Label).c_str()
#define CPPH_C_LOCTEXT(RefText)        INTERNALCPPH_LOCTEXT_FULL(RefText, RefText, nullptr).c_str()
#define CPPH_C_LOCWORD(RefText)        INTERNALCPPH_LOCTEXT_FULL(RefText, RefText, RefText).c_str()
