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

#include <streambuf>

#include "../algorithm/base64.hxx"
#include "__namespace__"

namespace CPPHEADERS_NS_::streambuf {

class b64 : public std::streambuf
{
    std::streambuf* _src;

    char _obuf[3];
    char _n_obuf = 0;

    char _ibuf[3];

   public:
    explicit b64(std::streambuf* adapted = nullptr) noexcept
            : _src(adapted)
    {
    }

    std::streambuf* reset(std::streambuf* adapted = nullptr) noexcept
    {
        if (_src) { sync(); }
        return std::swap(_src, adapted), adapted;
    }

   protected:
    int_type overflow(int_type ch) override
    {
        _obuf[_n_obuf++] = traits_type ::to_char_type(ch);
        if (_n_obuf == 3) { _write_word(); }

        return traits_type::to_int_type(ch);
    }

    int_type underflow() override
    {
        char buf[4];
        auto n_read = _src->sgetn(buf, 4);
        if (n_read != 4) { return traits_type::eof(); }

        auto n_decoded = base64::decoded_size(buf);
        base64::decode_bytes(buf, n_read, _ibuf);

        setg(_ibuf, _ibuf, _ibuf + n_decoded);
        return traits_type::to_int_type(_ibuf[0]);
    }

    int sync() override
    {
        if (not _src) { throw std::runtime_error{"source buffer not set"}; }

        if (_n_obuf > 0) { _write_word(); }
        return _src->pubsync();
    }

   private:
    void _write_word()
    {
        char encoded[4];
        base64::encode_bytes(_obuf, _n_obuf, encoded);
        _src->sputn(encoded, 4);
        _n_obuf = 0;
    }
};
}  // namespace CPPHEADERS_NS_::streambuf
