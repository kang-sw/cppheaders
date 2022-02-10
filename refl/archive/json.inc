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

#include <charconv>
#include <iterator>

#include "../../helper/strutil.hxx"
#include "../../streambuf/base64.hxx"
#include "../../streambuf/view.hxx"
#include "../../utility/inserter.hxx"
#include "json.hpp"

#define JSMN_STATIC
#define JSMN_STRICT
#include "../../third/jsmn.h"

namespace CPPHEADERS_NS_::archive::json {
using namespace std::literals;

writer::writer(std::streambuf& buf, size_t depth_maybe)
        : if_writer(buf), _base64(&buf)
{
    _ctx.reserve_depth(depth_maybe);
}

if_writer& writer::operator<<(double v)
{
    _on_write();

    char buf[32];
    auto r = snprintf(buf, sizeof buf, "%f", v);

    _buf->sputn(buf, r);
    return *this;
}

if_writer& writer::operator<<(int64_t v)
{
    _on_write();

    char buf[32];
    auto r = std::to_chars(buf, buf + sizeof buf, v);

    _buf->sputn(buf, r.ptr - buf);
    return *this;
}

if_writer& writer::operator<<(nullptr_t a_nullptr)
{
    _on_write();

    _buf->sputn("null", 4);
    return *this;
}

if_writer& writer::operator<<(std::string_view v)
{
    auto d = _ctx.write_next();
    if (d.need_comma) { _append_comma(); }
    if (d.need_indent) { _brk_indent(); }

    // TODO: Escape strings

    _buf->sputc('"');
    auto inserter = adapt_inserter([&](char ch) { _buf->sputc(ch); });
    strutil::escape(v.begin(), v.end(), inserter);
    _buf->sputc('"');

    if (d.is_key)
    {
        _buf->sputc(':');
        if (indent >= 0) { _buf->sputc(' '); }
    }
    return *this;
}

if_writer& writer::operator<<(bool v)
{
    _on_write();

    if (v)
        _buf->sputn("true", 4);
    else
        _buf->sputn("false", 5);

    return *this;
}

if_writer& writer::binary_push(size_t total)
{
    _on_write();

    _ctx.push_binary(total);
    _buf->sputc('"');
    return *this;
}

if_writer& writer::binary_write_some(const_buffer_view view)
{
    _ctx.binary_write_some(view.size());
    _base64.sputn(view.data(), view.size());
    return *this;
}

if_writer& writer::binary_pop()
{
    _ctx.pop_binary();
    _base64.pubsync();
    _buf->sputc('"');
    return *this;
}

if_writer& writer::object_push(size_t num_elems)
{
    _on_write();

    _ctx.push_object(num_elems);
    _buf->sputc('{');

    return *this;
}

if_writer& writer::object_pop()
{
    if (_ctx.pop_object() > 0) { _brk_indent(); }
    _buf->sputc('}');

    return *this;
}

if_writer& writer::array_push(size_t num_elems)
{
    _on_write();

    _ctx.push_array(num_elems);
    _buf->sputc('[');

    return *this;
}

if_writer& writer::array_pop()
{
    if (_ctx.pop_array() > 0) { _brk_indent(); }
    _buf->sputc(']');

    return *this;
}

void writer::write_key_next()
{
    _ctx.write_key_next();
}

void writer::_on_write()
{
    auto d = _ctx.write_next();
    if (d.is_key) { _throw_invalid_key_type(); }
    if (d.need_comma) { _append_comma(); }
    if (d.need_indent) { _brk_indent(); }
}

void writer::_append_comma()
{
    _buf->sputc(',');
}

void writer::_throw_invalid_key_type()
{
    throw invalid_key_type{this};
}

void writer::_brk_indent()
{
    // no indent
    if (indent < 0) { return; }

    _buf->sputc('\n');
    for (int i = 0; i < indent * _ctx.depth(); ++i) { _buf->sputc(' '); }
}

}  // namespace CPPHEADERS_NS_::archive::json

namespace CPPHEADERS_NS_::archive::json {

enum class reader_scope_type
{
    invalid,
    array,
    object
};

struct reader_scope_context_t
{
    reader_scope_type type = {};
    context_key context    = {};
    int token_pos          = false;
    bool is_key_next       = false;
    int elem_left          = 0;
};

struct reader::impl
{
    if_reader* self;

    std::vector<jsmntok_t> tokens;
    std::string string;
    jsmn_parser parser;
    size_t pos_next = ~size_t{};

    std::vector<reader_scope_context_t> scopes;
    int64_t context_keygen = 0;

    streambuf::view base64_view{};
    streambuf::b64 base64{&base64_view};

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

        auto elem           = &scopes.emplace_back();
        elem->type          = t;
        elem->token_pos     = pos_next++;
        elem->elem_left     = ntok->size * ((t == reader_scope_type::object) + 1);
        elem->context.value = ++context_keygen;

        return elem;
    }

    void step_over()
    {
        if (scopes.empty()) { return; }

        auto scope = &scopes.back();
        auto ntok  = next();

        if (scope->is_key_next && ntok->type != JSMN_STRING) { throw error::reader_invalid_context{self}; }

        if (ntok->type == JSMN_OBJECT || ntok->type == JSMN_ARRAY)
        {
            auto it = std::lower_bound(
                    tokens.begin() + pos_next, tokens.end(), ntok->end,
                    [](jsmntok_t const& t, int e) { return t.start < e; });
            pos_next = it - tokens.begin();
        }
        else
        {
            ++pos_next;
        }

        if (scope->elem_left-- <= 0)
            throw error::reader_invalid_context{self}.message("end of object");

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

        if (scopes.empty())
        {
            pos_next = ~size_t{};
        }
        else
        {
            scope = &scopes.back();
            --scope->elem_left;
            scope->is_key_next = false;
        }
    }
};

reader::reader(std::streambuf& buf)
        : if_reader(buf), self(new impl{this})
{
    reset(), _validate();
}

if_reader& reader::operator>>(nullptr_t a_nullptr)
{
    _validate();
    self->step_over();
    return *this;
}

if_reader& reader::operator>>(bool& v)
{
    _validate();
    auto next = self->next();
    auto tok  = self->tokstr(*next);
    if (next->type != JSMN_PRIMITIVE) { throw error::reader_parse_failed{this}; }

    if (tok == "true")
        v = true;
    else if (tok == "false")
        v = false;
    else
        throw error::reader_parse_failed{this}.message(tok);

    self->step_over();
    return *this;
}

if_reader& reader::operator>>(int64_t& v)
{
    _validate();

    auto next = self->next();
    auto tok  = self->tokstr(*next);
    if (next->type != JSMN_PRIMITIVE) { throw error::reader_parse_failed{this}; }

    auto r = std::from_chars(tok.data(), tok.data() + tok.size(), v);
    if (r.ptr != tok.data() + tok.size()) { throw error::reader_parse_failed{this}.message(tok); }

    self->step_over();
    return *this;
}

if_reader& reader::operator>>(double& v)
{
    _validate();

    auto next = self->next();
    auto tok  = self->tokstr(*next);
    if (next->type != JSMN_PRIMITIVE) { throw error::reader_parse_failed{this}; }

    char* eptr = {};
    v          = strtod(tok.data(), &eptr);
    if (eptr != tok.data() + tok.size()) { throw error::reader_parse_failed{this}.message(tok); }

    self->step_over();
    return *this;
}

if_reader& reader::operator>>(std::string& v)
{
    _validate();

    auto next = self->next();
    auto tok  = self->tokstr(*next);
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
    if (binsize & 0x3) { throw error::reader_parse_failed{this}.message("invalid base64 binary: %llu", binsize); }

    auto buffer = array_view<char>(self->string).subspan(next->start, binsize);
    self->base64_view.reset(buffer);

    return base64::decoded_size(buffer);
}

if_reader& reader::binary_read_some(mutable_buffer_view v)
{
    auto next = self->next();
    if (next->type != JSMN_STRING) { throw error::reader_parse_failed{this}; }

    // copy buffer
    auto n_read = self->base64.sgetn(v.data(), v.size());
    if (n_read != v.size()) { throw error::reader_parse_failed{this}.message("buffer size mismatch"); }

    return *this;
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

bool reader::is_null_next() const
{
    auto next = self->next();
    return next->type == JSMN_PRIMITIVE && self->tokstr(*next) == "null"sv;
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

    if (n_tok <= 0) { throw error::reader_parse_failed{this}.message("jsmn error: %d", n_tok); }

    jsmn_init(&self->parser);
    tok->resize(n_tok);
    auto n_proc = jsmn_parse(&self->parser, str->data(), str->size(), tok->data(), tok->size());

    assert(n_tok == n_proc);
    self->pos_next = 0;  // Ready to start parsing
}

bool reader::is_object_next() const
{
    return self->next()->type == JSMN_OBJECT;
}

bool reader::is_array_next() const
{
    return self->next()->type == JSMN_ARRAY;
}

reader::~reader() = default;

}  // namespace CPPHEADERS_NS_::archive::json