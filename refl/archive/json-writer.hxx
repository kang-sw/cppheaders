/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2021-2022. Seungwoo Kang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * project home: https://github.com/perfkitpp
 ******************************************************************************/

#pragma once
#include <charconv>
#include <sstream>

#include "../../helper/strutil.hxx"
#include "../../streambuf/base64.hxx"
#include "../../streambuf/view.hxx"
#include "../../utility/inserter.hxx"
#include "detail/context_helper.hxx"

namespace CPPHEADERS_NS_::archive::json {
CPPH_DECLARE_EXCEPTION(invalid_key_type, error::writer_exception);

class writer : public archive::if_writer
{
   private:
    detail::write_context_helper _ctx;

   public:
    int              indent = -1;
    streambuf::b64_w _base64;

   public:
    explicit writer(std::streambuf* buf, size_t depth_maybe = 0)
            : if_writer(buf), _base64(buf)
    {
        _ctx.reserve_depth(depth_maybe);
    }

    if_writer& write(nullptr_t a_nullptr) override
    {
        _on_write();

        sputn("null", 4);
        return *this;
    }

    if_writer& write(int64_t v) override
    {
        auto d = _ctx.write_next();
        if (d.need_comma) { _append_comma(); }
        if (d.need_indent) { _brk_indent(); }

        char buf[32];
        auto r = std::to_chars(buf, buf + sizeof buf, v);

        if (d.is_key) {
            sputc('"');
            sputn(buf, r.ptr - buf);
            sputc('"');

            sputc(':');
            if (indent >= 0) { sputc(' '); }
        } else {
            sputn(buf, r.ptr - buf);
        }

        return *this;
    }

    if_writer& write(double v) override
    {
        _on_write();

        char buf[32];
        auto r = snprintf(buf, sizeof buf, "%.14g", v);

        sputn(buf, r);
        return *this;
    }

    if_writer& write(bool v) override
    {
        _on_write();

        if (v)
            sputn("true", 4);
        else
            sputn("false", 5);

        return *this;
    }

    if_writer& write(std::string_view v) override
    {
        auto d = _ctx.write_next();
        if (d.need_comma) { _append_comma(); }
        if (d.need_indent) { _brk_indent(); }

        sputc('"');
        auto inserter = adapt_inserter([&](char ch) { sputc(ch); });
        strutil::escape(v.begin(), v.end(), inserter);
        sputc('"');

        if (d.is_key) {
            sputc(':');
            if (indent >= 0) { sputc(' '); }
        }
        return *this;
    }

    if_writer& binary_push(size_t total) override
    {
        _on_write();

        _ctx.push_binary(total);
        sputc('"');

        _base64.reset(_buf);
        return *this;
    }

    if_writer& binary_write_some(const_buffer_view view) override
    {
        _ctx.binary_write_some(view.size());
        _base64.sputn(view.data(), view.size());
        return *this;
    }

    if_writer& binary_pop() override
    {
        _ctx.pop_binary();
        _base64.pubsync();
        sputc('"');
        return *this;
    }

    if_writer& object_push(size_t num_elems) override
    {
        _on_write();

        _ctx.push_object(num_elems);
        sputc('{');

        return *this;
    }

    if_writer& object_pop() override
    {
        if (_ctx.pop_object() > 0) { _brk_indent(); }
        sputc('}');

        return *this;
    }

    if_writer& array_push(size_t num_elems) override
    {
        _on_write();

        _ctx.push_array(num_elems);
        sputc('[');

        return *this;
    }

    if_writer& array_pop() override
    {
        if (_ctx.pop_array() > 0) { _brk_indent(); }
        sputc(']');

        return *this;
    }

    void write_key_next() override
    {
        _ctx.write_key_next();
    }

   private:
    void _on_write()
    {
        auto d = _ctx.write_next();
        if (d.is_key) { _throw_invalid_key_type(); }
        if (d.need_comma) { _append_comma(); }
        if (d.need_indent) { _brk_indent(); }
    }

    void _append_comma()
    {
        sputc(',');
    }

    void _throw_invalid_key_type()
    {
        throw invalid_key_type{this};
    }

    void _brk_indent()
    {
        // no indent
        if (indent < 0) { return; }

        sputc('\n');
        for (int i = 0; i < indent * _ctx.depth(); ++i) { sputc(' '); }
    }
};

}  // namespace CPPHEADERS_NS_::archive::json