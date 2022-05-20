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
#include <sstream>

#include "../../streambuf/base64.hxx"
#include "../../streambuf/view.hxx"
#include "json-writer.hxx"

namespace cpph::archive::json {
class reader : public archive::if_reader
{
    struct impl;
    std::unique_ptr<impl> self;

   public:
    explicit reader(std::streambuf* buf, bool use_intkey = false);
    ~reader() override;

    //! Prepare for next list of tokens
    void reset();

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

   private:
    void _prepare() const;
};

}  // namespace cpph::archive::json

namespace cpph::archive {

template <typename ValTy_>
std::string to_json(ValTy_ const& value)
{
    std::stringbuf sstr;
    json::writer wr{&sstr, 8};
    wr << value;
    return sstr.str();
}

template <typename ValTy_>
void from_json(std::string_view str, ValTy_* ref)
{
    streambuf::view view{{(char*)str.data(), str.size()}};
    json::reader rd{&view};
    rd >> *ref;
}

}  // namespace cpph::archive
