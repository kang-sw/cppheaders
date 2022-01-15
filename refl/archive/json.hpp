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

#pragma once
#include <charconv>

#include "context_helper.hxx"

namespace CPPHEADERS_NS_::archive::json {
CPPH_DECLARE_EXCEPTION(invalid_key_type, error::writer_exception);

class writer : public archive::if_writer
{
   private:
    detail::write_context_helper _ctx;

   public:
    int indent = -1;

   public:
    explicit writer(std::streambuf& buf, size_t depth_maybe = 0) : if_writer(buf)
    {
        _ctx.reserve_depth(depth_maybe);
    }

    if_writer& operator<<(nullptr_t a_nullptr) override
    {
        auto d = _ctx.write_next();
        if (d.is_key) { _throw_invalid_key_type(); }
        if (d.need_comma) { _append_comma(); }
        if (d.need_indent) { _brk_indent(); }

        _buf->sputn("null", 4);
        return *this;
    }

    if_writer& operator<<(int64_t v) override
    {
        auto d = _ctx.write_next();
        if (d.is_key) { _throw_invalid_key_type(); }
        if (d.need_comma) { _append_comma(); }
        if (d.need_indent) { _brk_indent(); }

        char buf[32];
        auto r = std::to_chars(buf, buf + sizeof buf, v);

        _buf->sputn(buf, r.ptr - buf);
        return *this;
    }

    if_writer& operator<<(double v) override
    {
        auto d = _ctx.write_next();
        if (d.is_key) { _throw_invalid_key_type(); }
        if (d.need_comma) { _append_comma(); }
        if (d.need_indent) { _brk_indent(); }

        char buf[32];
        auto r = std::to_chars(buf, buf + sizeof buf, v);

        _buf->sputn(buf, r.ptr - buf);
        return *this;
    }

    if_writer& operator<<(std::string_view v) override
    {
        auto d = _ctx.write_next();
        if (d.need_comma) { _append_comma(); }
        if (d.need_indent) { _brk_indent(); }

        _buf->sputc('"');
        _buf->sputn(v.data(), v.size());
        _buf->sputc('"');

        if (d.is_key) { _buf->sputc(':'); }
        return *this;
    }

    if_writer& binary_push(size_t total) override
    {
        auto d = _ctx.write_next();
        if (d.is_key) { _throw_invalid_key_type(); }
        if (d.need_comma) { _append_comma(); }
        if (d.need_indent) { _brk_indent(); }

        _ctx.push_binary(total);
        _buf->sputc('"');
        return *this;
    }

    if_writer& binary_write_some(const_buffer_view view) override
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

    if_writer& binary_pop() override
    {
        _ctx.pop_binary();
        _buf->sputc('"');
        return *this;
    }

    if_writer& object_push(size_t num_elems) override
    {
        auto d = _ctx.write_next();
        if (d.is_key) { _throw_invalid_key_type(); }
        if (d.need_comma) { _append_comma(); }
        if (d.need_indent) { _brk_indent(); }

        _ctx.push_object(num_elems);
        _buf->sputc('{');

        return *this;
    }

    if_writer& object_pop() override
    {
        if (_ctx.pop_object() > 0) { _brk_indent(); }
        _buf->sputc('}');

        return *this;
    }

    if_writer& array_push(size_t num_elems) override
    {
        auto d = _ctx.write_next();
        if (d.is_key) { _throw_invalid_key_type(); }
        if (d.need_comma) { _append_comma(); }
        if (d.need_indent) { _brk_indent(); }

        _ctx.push_array(num_elems);
        _buf->sputc('[');

        return *this;
    }

    if_writer& array_pop() override
    {
        if (_ctx.pop_array() > 0) { _brk_indent(); }
        _buf->sputc(']');

        return *this;
    }

    void write_key_next() override
    {
        _ctx.write_key_next();
    }

   private:
    void _append_comma()
    {
        _buf->sputc(',');
    }

    void _throw_invalid_key_type()
    {
        throw invalid_key_type{this};
    }

    void _brk_indent()
    {
        // no indent
        if (indent < 0) { return; }

        _buf->sputc('\n');
        for (int i = 0; i < indent * _ctx.depth(); ++i) { _buf->sputc(' '); }
    }
};

}  // namespace CPPHEADERS_NS_::archive::json
