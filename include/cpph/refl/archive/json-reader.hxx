
#pragma once
#include <cpph/std/string_view>

#include "../../streambuf/base64.hxx"
#include "../../streambuf/view.hxx"
#include "cpph/refl/detail/if_archive.hxx"
namespace cpph::archive::json {
using std::string;
using std::string_view;

class reader : public archive::if_reader
{
    struct impl;
    std::unique_ptr<impl> self;

   public:
    explicit reader(std::streambuf* buf, bool use_intkey = false);
    ~reader() override;

    //! Prepare for next list of tokens
    void reset();
    void clear() override { reset(), if_reader::clear(); }

    //! Validate internal state.
    //! Reads content from buffer.
    void validate() { _prepare(); }

   public:
    if_reader& read(nullptr_t a_nullptr) override;
    if_reader& read(bool& v) override;
    if_reader& read(int64_t& v) override;
    if_reader& read(double& v) override;
    if_reader& read(std::string& v) override;
    size_t elem_left() const override;
    size_t begin_binary() override;
    size_t binary_read_some(mutable_buffer_view v) override;
    void end_binary() override;
    context_key begin_object() override;
    context_key begin_array() override;
    bool should_break(const context_key& key) const override;
    void end_object(context_key key) override;
    void end_array(context_key key) override;
    void read_key_next() override;
    entity_type type_next() const override;
    bool goto_key(string_view key);

   private:
    void _prepare() const;
};

}  // namespace cpph::archive::json

#include <charconv>
#include <iterator>

#include "cpph/helper/strutil.hxx"
#include "cpph/streambuf/base64.hxx"
#include "cpph/streambuf/view.hxx"
#include "cpph/third/jsmn.h"
#include "cpph/utility/inserter.hxx"

namespace cpph::archive::json {
using namespace std::literals;

enum class reader_scope_type {
    invalid,
    array,
    object
};

struct reader_scope_context_t {
    reader_scope_type const type;
    context_key const context;
    int const token_pos;
    bool is_key_next;
    int elem_left;
};

struct reader::impl {
    if_reader* self;

    std::vector<_jsmn::jsmntok_t> tokens;
    std::string buffer;
    _jsmn::jsmn_parser parser;
    size_t pos_next = ~size_t{};

    std::vector<reader_scope_context_t> scopes;
    int64_t context_keygen = 0;

    streambuf::view base64_view{};
    streambuf::b64_r base64{&base64_view};

   public:
    auto next() const { return &tokens.at(pos_next); }
    auto next() { return &tokens.at(pos_next); }

    std::string_view tokstr(_jsmn::jsmntok_t const& tok) const
    {
        std::string_view view{buffer};
        return view.substr(tok.start, tok.end - tok.start);
    }

    reader_scope_context_t const* step_in(reader_scope_type t)
    {
        auto ntok = next();

        if (t == reader_scope_type::object && ntok->type != _jsmn::JSMN_OBJECT)
            throw error::reader_parse_failed{self};
        if (t == reader_scope_type::array && ntok->type != _jsmn::JSMN_ARRAY)
            throw error::reader_parse_failed{self};

        auto elem = &scopes.emplace_back(
                reader_scope_context_t{
                        t,
                        {context_keygen},
                        (int)pos_next++,
                        false,
                        ntok->size * (t == reader_scope_type::object ? 2 : 1),
                });

        return elem;
    }

    void step()
    {
        if (scopes.empty()) { return; }

        auto scope = &scopes.back();
        auto ntok = next();

        if (scope->is_key_next && ntok->type != _jsmn::JSMN_STRING) { throw error::reader_invalid_context{self}; }

        if (ntok->type == _jsmn::JSMN_OBJECT || ntok->type == _jsmn::JSMN_ARRAY) {
            pos_next = step_over(pos_next);
        } else {
            ++pos_next;
        }

        if (pos_next == tokens.size()) {
            pos_next = ~size_t{};
        }

        if (scope->elem_left-- <= 0)
            throw error::reader_invalid_context{self, "end of object"};

        scope->is_key_next = false;
    }

    void step_out(context_key key)
    {
        if (scopes.empty()) { throw error::reader_invalid_context{self}; }

        auto scope = &scopes.back();
        if (scope->context.value != key.value) { throw error::reader_invalid_context{self}; }
        if (scope->is_key_next) { throw error::reader_invalid_context{self}; }

        auto it_tok = tokens.begin() + scope->token_pos;

        auto it = std::lower_bound(
                it_tok, tokens.end(), it_tok->end,
                [](_jsmn::jsmntok_t const& t, int e) { return t.start < e; });
        pos_next = it - tokens.begin();
        scopes.pop_back();

        if (scopes.empty()) {
            pos_next = ~size_t{};
        } else {
            scope = &scopes.back();
            --scope->elem_left;
            scope->is_key_next = false;
        }
    }

    int step_over(int tokidx) const
    {
        auto it_tok = tokens.begin() + tokidx;
        auto iter = std::lower_bound(
                it_tok, tokens.end(), it_tok->end,
                [](_jsmn::jsmntok_t const& tok, int end) { return tok.start < end; });

        return iter - tokens.begin();
    }
};

inline reader::reader(std::streambuf* buf, bool use_intkey)
        : if_reader(buf), self(new impl{this})
{
    config.use_integer_key = use_intkey;
    reset();
}

inline if_reader& reader::read(nullptr_t a_nullptr)
{
    _prepare();
    self->step();
    return *this;
}

inline if_reader& reader::read(bool& v)
{
    _prepare();
    auto next = self->next();
    auto tok = self->tokstr(*next);
    if (next->type != _jsmn::JSMN_PRIMITIVE) { throw error::reader_parse_failed{this}; }

    if (tok == "true")
        v = true;
    else if (tok == "false")
        v = false;
    else
        throw error::reader_parse_failed{this};

    self->step();
    return *this;
}

inline if_reader& reader::read(int64_t& v)
{
    _prepare();

    auto next = self->next();
    auto tok = self->tokstr(*next);
    if (next->type == _jsmn::JSMN_STRING || next->type == _jsmn::JSMN_PRIMITIVE) {
        auto r = std::from_chars(tok.data(), tok.data() + tok.size(), v);
        if (r.ptr != tok.data() + tok.size()) { throw error::reader_parse_failed{this}; }
    } else {
        throw error::reader_parse_failed{this};
    }

    self->step();
    return *this;
}

inline if_reader& reader::read(double& v)
{
    _prepare();

    auto next = self->next();
    auto tok = self->tokstr(*next);
    if (next->type == _jsmn::JSMN_STRING || next->type == _jsmn::JSMN_PRIMITIVE) {
        char* eptr = {};
        v = strtod(tok.data(), &eptr);
        if (eptr != tok.data() + tok.size()) { throw error::reader_parse_failed{this}; }
    } else {
        throw error::reader_parse_failed{this};
    }

    self->step();
    return *this;
}

inline if_reader& reader::read(std::string& v)
{
    _prepare();

    auto next = self->next();
    auto tok = self->tokstr(*next);
    if (next->type != _jsmn::JSMN_STRING) { throw error::reader_parse_failed{this}; }

    v.clear();
    strutil::json_unescape(tok.begin(), tok.end(), std::back_inserter(v));

    self->step();
    return *this;
}

inline size_t reader::elem_left() const
{
    return self->scopes.back().elem_left;
}

inline size_t reader::begin_binary()
{
    _prepare();

    auto next = self->next();
    if (next->type != _jsmn::JSMN_STRING) { throw error::reader_parse_failed{this}; }

    auto binsize = (next->end - next->start);
    if (binsize & 1) { throw error::reader_parse_failed{this}; }
    if (binsize & 0x3) { throw error::reader_parse_failed{this, "invalid base64 binary: %llu", binsize}; }

    auto buffer = array_view<char>(self->buffer).subspan(next->start, binsize);
    self->base64_view.reset(buffer);

    return base64::decoded_size(buffer);
}

inline size_t reader::binary_read_some(mutable_buffer_view v)
{
    auto next = self->next();
    if (next->type != _jsmn::JSMN_STRING) { throw error::reader_parse_failed{this}; }

    // copy buffer
    auto n_read = self->base64.sgetn(v.data(), v.size());
    if (n_read == EOF) { return 0; }

    return n_read;
}

inline void reader::end_binary()
{
    self->step();
}

inline context_key reader::begin_object()
{
    _prepare();
    return self->step_in(reader_scope_type::object)->context;
}

inline context_key reader::begin_array()
{
    _prepare();
    return self->step_in(reader_scope_type::array)->context;
}

inline bool reader::should_break(const context_key& key) const
{
    auto& top = self->scopes.back();
    if (top.context.value != key.value) { return false; }

    return top.elem_left == 0;
}

inline void reader::end_object(context_key key)
{
    self->step_out(key);
}

inline void reader::end_array(context_key key)
{
    self->step_out(key);
}

inline void reader::read_key_next()
{
    auto& top = self->scopes.back();
    if (top.is_key_next) { throw error::reader_invalid_context{this}; }
    if (top.type != reader_scope_type::object) { throw error::reader_parse_failed{this}; }

    top.is_key_next = true;
}

inline void reader::reset()
{
    self->pos_next = ~size_t{};
}

inline void reader::_prepare() const
{
    // data is ready
    if (self->pos_next != ~size_t{}) { return; }

    // Clear state
    self->scopes.clear();

    // Read until EOF, and try parse
    auto str = &self->buffer;
    auto tokens = &self->tokens;
    auto parser = &self->parser;
    str->clear();
    tokens->resize(8);

    // Read string from buffer, for single object next.
    _jsmn::jsmn_init(parser);

    // Try read first token
    while (parser->toknext == 0) {
        auto c = _buf->sbumpc();
        if (c == EOF) { throw error::reader_unexpected_end_of_file{this}; }

        str->push_back(c);
        switch (_jsmn::jsmn_parse(parser, str->c_str(), str->size(), tokens->data(), tokens->size())) {
            case _jsmn::JSMN_ERROR_INVAL:
                throw error::reader_parse_failed{this};

            case _jsmn::JSMN_ERROR_NOMEM:
            case 0:
                assert(false && "Logically impossible!"), abort();

            default: break;
        }
    }

    char stop_char = 0;
    switch ((*tokens)[0].type) {
        case _jsmn::JSMN_OBJECT: stop_char = '}'; break;
        case _jsmn::JSMN_ARRAY: stop_char = ']'; break;

        case _jsmn::JSMN_STRING:
        case _jsmn::JSMN_PRIMITIVE:  // Primitive root can only have single argument
            self->pos_next = 0;
            tokens->resize(1);
            return;

        default: throw error::reader_invalid_context{this};
    }

    // Parse rest of the object
    int r_parse = 0;
    for (;;) {
        auto c = _buf->sbumpc();
        if (c == EOF) { throw error::reader_unexpected_end_of_file{this}; }

        str->push_back(c);
        if (c != stop_char) { continue; }  // Just keep reading ...

    // On stop char, try parsing
    PARSE_AGAIN:
        r_parse = _jsmn::jsmn_parse(parser, str->c_str(), str->size(), tokens->data(), tokens->size());

        switch (r_parse) {
            case 0:
            case _jsmn::JSMN_ERROR_INVAL:
                throw error::reader_parse_failed{this};

            case _jsmn::JSMN_ERROR_NOMEM:
                tokens->resize(tokens->size() * 3 / 2);
                goto PARSE_AGAIN;

            case _jsmn::JSMN_ERROR_PART:
                break;

            default:
                self->pos_next = 0;
                tokens->resize(r_parse);  // shrink to actual token size
                return;
        }
    }
}

inline entity_type reader::type_next() const
{
    _prepare();
    auto next = self->next();

    switch (next->type) {
        case _jsmn::JSMN_STRING:
            return entity_type::string;

        case _jsmn::JSMN_PRIMITIVE: {
            auto tok = self->tokstr(*next);
            if (tok == "null"sv) { return entity_type::null; }
            if (tok[0] == 't' || tok[0] == 'f') { return entity_type::boolean; }

            auto has_dot = tok.find_first_of("eE.") != std::string::npos;
            return has_dot ? entity_type::floating_point : entity_type::integer;
        }

        case _jsmn::JSMN_ARRAY:
            return entity_type::array;

        case _jsmn::JSMN_OBJECT:
            return entity_type::object;

        default:
        case _jsmn::JSMN_UNDEFINED:
            throw error::reader_invalid_context{this, "_jsmn::jsmn error: invalid next type"};
    }
}

inline bool reader::goto_key(string_view key)
{
    if (self->scopes.empty())
        throw error::reader_check_failed{this, "goto_key() called from empty scope context"};

    auto& scope = self->scopes.back();
    if (scope.type != reader_scope_type::object)
        throw error::reader_check_failed{this, "goto_key() called from non-object context"};

    auto const parent_pos = scope.token_pos;
    auto const parent_token = self->tokens.begin() + parent_pos;
    auto key_left = parent_token->size;

    assert(parent_token->type == _jsmn::JSMN_OBJECT);

    if (key_left == 0) { return false; }  // Empty object.

    for (auto cursor = parent_pos + 1; key_left--; cursor = self->step_over(cursor + 1)) {
        auto content = self->tokstr(self->tokens[cursor]);
        assert(self->tokens[cursor].type == _jsmn::JSMN_STRING);
        assert(self->tokens[cursor].parent == parent_pos);

        if (content == key) {
            self->pos_next = cursor + 1;
            scope.elem_left = key_left * 2 + 1;
            scope.is_key_next = false;

            return true;
        }
    }

    return false;
}

inline reader::~reader() = default;
}  // namespace cpph::archive::json
