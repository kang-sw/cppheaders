#include "cpph/app/localize.hpp"

#include <cpph/std/algorithm>
#include <cpph/std/map>
#include <cpph/std/unordered_map>
#include <filesystem>
#include <fstream>
#include <shared_mutex>

#include "cpph/algorithm/base64.hxx"
#include "cpph/refl/archive/json.hpp"
#include "cpph/refl/object.hxx"
#include "cpph/refl/types/binary.hxx"
#include "cpph/refl/types/tuple.hxx"
#include "cpph/refl/types/unordered.hxx"
#include "cpph/thread/event_wait.hxx"
#include "cpph/thread/locked.hxx"
#include "cpph/thread/thread_pool.hxx"
#include "cpph/utility/singleton.hxx"

//
using namespace cpph;

namespace std {
template <>
struct hash<chunk<uint64_t>> {
    size_t operator()(chunk<uint64_t> value) const noexcept
    {
        return std::hash<uint64_t>{}(value.value);
    }
};
}  // namespace std

namespace cpph {
namespace _detail {

struct loca_text_entity {
    CPPH_REFL_DEFINE_OBJECT_inline_simple(label, content);

    string content;
    optional<string> label;
};

using loca_full_lut_t = unordered_map<uint64_t, loca_text_entity>;
using loca_archived_lut_t = unordered_map<chunk<uint64_t>, loca_text_entity>;
using loca_mutable_lut_t = unordered_map<uint64_t, string>;
using loca_lut_t = const unordered_map<uint64_t, string>;

static auto acquire_global_builder() { return default_singleton<locked<loca_full_lut_t>, class TMP_0>().lock(); }

static auto acquire_loaded_tables() { return default_singleton<locked<map<string, loca_lut_t*, std::less<>>>>().lock(); }

static atomic<loca_lut_t*> loca_active_lut_table = nullptr;

static string const* lookup(uint64_t hash)
{
    if (auto lut = relaxed(loca_active_lut_table))
        if (auto pair = find_ptr(*lut, hash))
            return &pair->second;

    return nullptr;
}

static void send_string_to_builder(uint64_t hash, string_view refstr, string_view label)
{
    auto builder = acquire_global_builder();
    auto [iter, is_first] = builder->try_emplace(hash);

    if (is_first) {
        iter->second.content = refstr;
        if (not label.empty() && refstr != label) { iter->second.label.emplace(label); }
    }
}

struct loca_static_context {
    uint64_t const hash;
    string const ref_text;
    atomic<loca_lut_t*> active_lut;
    atomic<string const*> cached_lookup;
};

loca_static_context* loca_create_static_context(uint64_t hash, const char* ref_text, const char* label) noexcept
{
    auto ctx{new loca_static_context{hash, ref_text, nullptr, nullptr}};

    if (not lookup(hash)) {
        send_string_to_builder(hash, ref_text, label ? label : string_view{});
    }

    return ctx;
}

string const& loca_lookup(loca_static_context* ctx) noexcept
{
    auto lut = relaxed(loca_active_lut_table);

    if (relaxed(loca_active_lut_table) != relaxed(ctx->active_lut)) {
        relaxed(ctx->active_lut, lut);

        loca_lut_t::const_pointer p_str = nullptr;
        if (lut && (p_str = find_ptr(*lut, ctx->hash))) {
            relaxed(ctx->cached_lookup, &p_str->second);
        }
    }

    if (auto lookup = relaxed(ctx->cached_lookup)) {
        return *lookup;
    } else {
        return ctx->ref_text;
    }
}

}  // namespace _detail

localization_result load_localization_lut(string_view key, archive::if_reader* strm)
{
    // Load struct
    _detail::loca_archived_lut_t table;
    try {
        *strm >> table;
    } catch (archive::error::reader_exception&) {
        return localization_result::invalid_content;
    }

    // Build LUT
    auto lut = make_unique<_detail::loca_mutable_lut_t>();
    lut->reserve(table.size());
    for (auto& [hash, content] : table) {
        lut->try_emplace(hash.value, content.content);
    }

    // Check if table with same-key is already loaded ... As language tables are not unloaded,
    //  not any language table with same key is permitted to be loaded multiple times.
    {
        auto globals = _detail::acquire_loaded_tables();
        if (find_ptr(*globals, key)) {
            return localization_result::already_loaded;
        } else {
            globals->try_emplace(string{key}, lut.release());
        }
    }

    // Send text to global builder
    {
        auto builder = _detail::acquire_global_builder();
        builder->merge(reinterpret_cast<_detail::loca_full_lut_t&&>(move(table)));
    }

    return localization_result::okay;
}

localization_result dump_localization_lut(archive::if_writer* strm)
{
    auto table = *_detail::acquire_global_builder();

    *strm << reinterpret_cast<_detail::loca_archived_lut_t&>(table);
    return localization_result::okay;
}

localization_result select_locale(string_view key)
{
    auto globals = _detail::acquire_loaded_tables();
    if (auto existing_table = find_ptr(*globals, key)) {
        release(_detail::loca_active_lut_table, existing_table->second);
        return localization_result::okay;
    } else {
        return localization_result::locale_not_loaded;
    }
}

localization_result load_localization_lut(string_view key, string const& path)
{
    std::ifstream fs{path};
    if (not fs.is_open()) { return localization_result::invalid_file_path; }

    archive::json::reader rd{fs.rdbuf()};
    return load_localization_lut(key, &rd);
}

localization_result dump_localization_lut(string_view path)
{
    std::filesystem::path dst = path;
    std::error_code ec;
    std::filesystem::create_directories(dst.parent_path(), ec);
    if (ec) { return localization_result::invalid_file_path; }

    std::ofstream fs{dst};
    if (not fs.is_open()) { return localization_result::invalid_file_path; }

    archive::json::writer wr{fs.rdbuf()};
    wr.indent = 2;
    return dump_localization_lut(&wr);
}
}  // namespace cpph
