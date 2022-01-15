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
    explicit writer(std::streambuf& buf, size_t depth_maybe = 0);

    if_writer& operator<<(nullptr_t a_nullptr) override;
    if_writer& operator<<(int64_t v) override;
    if_writer& operator<<(double v) override;
    if_writer& operator<<(bool v) override;
    if_writer& operator<<(std::string_view v) override;

    if_writer& binary_push(size_t total) override;
    if_writer& binary_write_some(const_buffer_view view) override;
    if_writer& binary_pop() override;

    if_writer& object_push(size_t num_elems) override;
    if_writer& object_pop() override;

    if_writer& array_push(size_t num_elems) override;
    if_writer& array_pop() override;

    void write_key_next() override;

   private:
    void _on_write();
    void _append_comma();
    void _throw_invalid_key_type();
    void _brk_indent();
};

}  // namespace CPPHEADERS_NS_::archive::json
