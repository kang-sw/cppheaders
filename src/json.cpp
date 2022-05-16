// MIT License
//
// Copyright (c) 2021-2022. Seungwoo Kang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// project home: https://github.com/perfkitpp

#include "cpph/refl/archive/json.hpp"

#include <charconv>
#include <iterator>

#include "cpph/helper/strutil.hxx"
#include "cpph/streambuf/base64.hxx"
#include "cpph/streambuf/view.hxx"
#include "cpph/utility/inserter.hxx"

#define JSMN_STATIC
#define JSMN_STRICT
#define JSMN_PARENT_LINKS
#include "cpph/third/jsmn.h"

#define CPPHEADERS_IMPLEMENT_ASSERTIONS
#include "cpph/assert.hxx"

namespace cpph::archive::json {
using namespace std::literals;

enum class reader_scope_type {
    invalid,
    array,
    object
};

struct reader_scope_context_t {
    reader_scope_type type = {};
    context_key context = {};
    int token_pos = false;
    bool is_key_next = false;
    int elem_left = 0;
};

struct reader::impl {
    if_reader* self;

    std::vector<jsmntok_t> tokens;
    std::string string;
    jsmn_parser parser;
    size_t pos_next = ~size_t{};

    std::vector<reader_scope_context_t> scopes;
    int64_t context_keygen = 0;

    streambuf::view base64_view{};
    streambuf::b64_r base64{&base64_view};

   public:
    auto next() const { return &tokens.at(pos_next); }
    auto next() { return &tokens.at(pos_next); }

    std::string_view tokstr(jsmntok_t const& tok) const
    {
        std::string_view view{string};
        return view.substr(tok.start, tok.end - tok.start);
    }

    std::string_view next_value_token() const
    {
        return "";
    }

    reader_scope_context_t const* step_in(reader_scope_type t)
    {
        auto ntok = next();

        if (t == reader_scope_type::object && ntok->type != JSMN_OBJECT)
            throw error::reader_parse_failed{self};
        if (t == reader_scope_type::array && ntok->type != JSMN_ARRAY)
            throw error::reader_parse_failed{self};

        auto elem = &scopes.emplace_back();
        elem->type = t;
        elem->token_pos = pos_next++;
        elem->elem_left = ntok->size * (t == reader_scope_type::object ? 2 : 1);
        elem->context.value = ++context_keygen;

        return elem;
    }

    void step_over()
    {
        if (scopes.empty()) { return; }

        auto scope = &scopes.back();
        auto ntok = next();

        if (scope->is_key_next && ntok->type != JSMN_STRING) { throw error::reader_invalid_context{self}; }

        if (ntok->type == JSMN_OBJECT || ntok->type == JSMN_ARRAY) {
            auto it = std::lower_bound(
                    tokens.begin() + pos_next, tokens.end(), ntok->end,
                    [](jsmntok_t const& t, int e) { return t.start < e; });
            pos_next = it - tokens.begin();
        } else {
            ++pos_next;
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
                [](jsmntok_t const& t, int e) { return t.start < e; });
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
};

reader::reader(std::streambuf* buf, bool use_intkey)
        : if_reader(buf), self(new impl{this})
{
    config.use_integer_key = use_intkey;
    reset(), _validate();
}

if_reader& reader::read(nullptr_t a_nullptr)
{
    _validate();
    self->step_over();
    return *this;
}

if_reader& reader::read(bool& v)
{
    _validate();
    auto next = self->next();
    auto tok = self->tokstr(*next);
    if (next->type != JSMN_PRIMITIVE) { throw error::reader_parse_failed{this}; }

    if (tok == "true")
        v = true;
    else if (tok == "false")
        v = false;
    else
        throw error::reader_parse_failed{this};

    self->step_over();
    return *this;
}

if_reader& reader::read(int64_t& v)
{
    _validate();

    auto next = self->next();
    auto tok = self->tokstr(*next);
    if (next->type == JSMN_STRING || next->type == JSMN_PRIMITIVE) {
        auto r = std::from_chars(tok.data(), tok.data() + tok.size(), v);
        if (r.ptr != tok.data() + tok.size()) { throw error::reader_parse_failed{this}; }
    } else {
        throw error::reader_parse_failed{this};
    }

    self->step_over();
    return *this;
}

if_reader& reader::read(double& v)
{
    _validate();

    auto next = self->next();
    auto tok = self->tokstr(*next);
    if (next->type == JSMN_STRING || next->type == JSMN_PRIMITIVE) {
        char* eptr = {};
        v = strtod(tok.data(), &eptr);
        if (eptr != tok.data() + tok.size()) { throw error::reader_parse_failed{this}; }
    } else {
        throw error::reader_parse_failed{this};
    }

    self->step_over();
    return *this;
}

if_reader& reader::read(std::string& v)
{
    _validate();

    auto next = self->next();
    auto tok = self->tokstr(*next);
    if (next->type != JSMN_STRING) { throw error::reader_parse_failed{this}; }

    v.clear();
    strutil::unescape(tok.begin(), tok.end(), std::back_inserter(v));

    self->step_over();
    return *this;
}

size_t reader::elem_left() const
{
    return self->scopes.back().elem_left;
}

size_t reader::begin_binary()
{
    auto next = self->next();
    if (next->type != JSMN_STRING) { throw error::reader_parse_failed{this}; }

    auto binsize = (next->end - next->start);
    if (binsize & 1) { throw error::reader_parse_failed{this}; }
    if (binsize & 0x3) { throw error::reader_parse_failed{this, "invalid base64 binary: %llu", binsize}; }

    auto buffer = array_view<char>(self->string).subspan(next->start, binsize);
    self->base64_view.reset(buffer);

    return base64::decoded_size(buffer);
}

size_t reader::binary_read_some(mutable_buffer_view v)
{
    auto next = self->next();
    if (next->type != JSMN_STRING) { throw error::reader_parse_failed{this}; }

    // copy buffer
    auto n_read = self->base64.sgetn(v.data(), v.size());
    if (n_read == EOF) { return 0; }

    return n_read;
}

void reader::end_binary()
{
    self->step_over();
}

context_key reader::begin_object()
{
    return self->step_in(reader_scope_type::object)->context;
}

context_key reader::begin_array()
{
    return self->step_in(reader_scope_type::array)->context;
}

bool reader::should_break(const context_key& key) const
{
    auto& top = self->scopes.back();
    if (top.context.value != key.value) { return false; }

    return top.elem_left == 0;
}

void reader::end_object(context_key key)
{
    self->step_out(key);
}

void reader::end_array(context_key key)
{
    self->step_out(key);
}

void reader::read_key_next()
{
    auto& top = self->scopes.back();
    if (top.is_key_next) { throw error::reader_invalid_context{this}; }
    if (top.type != reader_scope_type::object) { throw error::reader_parse_failed{this}; }

    top.is_key_next = true;
}

void reader::reset()
{
    self->pos_next = ~size_t{};
}

void reader::_validate()
{
    // data is ready
    if (self->pos_next != ~size_t{}) { return; }

    // Clear state
    self->scopes.clear();

    // Read until EOF, and try parse
    auto str = &self->string;
    auto tok = &self->tokens;

    str->clear();
    for (char c; (c = _buf->sbumpc()) != EOF;) { str->push_back(c); }

    jsmn_init(&self->parser);
    auto n_tok = jsmn_parse(&self->parser, str->data(), str->size(), nullptr, 0);

    if (n_tok <= 0) { throw error::reader_parse_failed{this, "jsmn error: %d", n_tok}; }

    jsmn_init(&self->parser);
    tok->resize(n_tok);
    auto n_proc = jsmn_parse(&self->parser, str->data(), str->size(), tok->data(), tok->size());

    assert(n_tok == n_proc);
    self->pos_next = 0;  // Ready to start parsing
}

entity_type reader::type_next() const
{
    auto next = self->next();

    switch (next->type) {
        case JSMN_STRING:
            return entity_type::string;

        case JSMN_PRIMITIVE: {
            auto tok = self->tokstr(*next);
            if (tok == "null"sv) { return entity_type::null; }
            if (tok[0] == 't' || tok[0] == 'f') { return entity_type::boolean; }

            auto has_dot = tok.find_first_of('.') == std::string::npos;
            return has_dot ? entity_type::floating_point : entity_type::integer;
        }

        case JSMN_ARRAY:
            return entity_type::array;

        case JSMN_OBJECT:
            return entity_type::object;

        default:
        case JSMN_UNDEFINED:
            throw error::reader_invalid_context{this, "jsmn error: invalid next type"};
    }
}

reader::~reader() = default;

}  // namespace cpph::archive::json