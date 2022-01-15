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

#include "json.hpp"

#define JSMN_STATIC
#define JSMN_PARENT_LINKS
#define JSMN_STRICT
#include "../../third/jsmn.h"

namespace CPPHEADERS_NS_::archive::json {

writer::writer(std::streambuf& buf, size_t depth_maybe) : if_writer(buf)
{
    _ctx.reserve_depth(depth_maybe);
}

if_writer& writer::operator<<(double v)
{
    _on_write();

    char buf[32];
    auto r = std::to_chars(buf, buf + sizeof buf, v);

    _buf->sputn(buf, r.ptr - buf);
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
    _buf->sputn(v.data(), v.size());
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

    constexpr auto table = "0123456789abcdef";

    for (uint8_t c : view)
    {
        _buf->sputc(table[(c >> 4) & 0xf]);
        _buf->sputc(table[(c >> 0) & 0xf]);
    }

    return *this;
}

if_writer& writer::binary_pop()
{
    _ctx.pop_binary();
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

struct reader::impl
{
    std::vector<jsmntok_t> tokens;
    std::string string;
    jsmn_parser parser;
    size_t pos_next = ~size_t{};
};

reader::reader(std::streambuf& buf) : if_reader(buf), self(new impl) { reset(), _validate(); }

if_reader& reader::operator>>(nullptr_t a_nullptr)
{
    _validate();
    return *this;
}

if_reader& reader::operator>>(bool& v)
{
    _validate();

    return *this;
}

if_reader& reader::operator>>(int64_t& v)
{
    _validate();

    return *this;
}

if_reader& reader::operator>>(double& v)
{
    _validate();

    return *this;
}

if_reader& reader::operator>>(std::string& v)
{
    _validate();

    return *this;
}

size_t reader::elem_left() const
{
    return 0;
}

size_t reader::begin_binary()
{
    return 0;
}

if_reader& reader::binary_read_some(mutable_buffer_view v)
{
    return *this;
}

void reader::end_binary()
{
}

context_key reader::begin_object()
{
    return context_key();
}

context_key reader::begin_array()
{
    return context_key();
}

bool reader::should_break(const context_key& key) const
{
    return false;
}

void reader::end_object(context_key key)
{
}

void reader::end_array(context_key key)
{
}

bool reader::read_key_next() const
{
    return false;
}

bool reader::is_null_next() const
{
    return false;
}

void reader::reset()
{
    self->pos_next = ~size_t{};
}

void reader::_validate()
{
    // data is ready
    if (self->pos_next != ~size_t{}) { return; }
    if (_buf->in_avail() <= 0) { throw error::reader_read_stream_error{this}.message("stream is empty!"); }

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
    return false;
}

bool reader::is_array_next() const
{
    return false;
}

reader::~reader() = default;

}  // namespace CPPHEADERS_NS_::archive::json