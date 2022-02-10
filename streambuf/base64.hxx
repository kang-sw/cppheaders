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

template <size_t WriteWords_ = 1, size_t ReadWords_ = 1>
class b64 : public std::streambuf
{
    std::streambuf* _src;

    enum
    {
        n_write_words = std::max<size_t>(1, WriteWords_),
        n_read_words  = std::max<size_t>(1, ReadWords_),
    };
    char _obuf[n_write_words * 3];
    char _ibuf[n_read_words * 3];

   public:
    explicit b64(std::streambuf* adapted = nullptr) noexcept
            : _src(adapted)
    {
        setp(_obuf, _obuf + sizeof _obuf);
    }

    std::streambuf* reset(std::streambuf* adapted = nullptr) noexcept
    {
        if (_src) { sync(); }
        return std::swap(_src, adapted), adapted;
    }

    ~b64() noexcept override
    {
        if (_src && _n_obuf() > 0) { _write_word(); }
    }

   protected:
    int_type overflow(int_type ch) override
    {
        if (_n_obuf() != 0) { _write_word(); }

        _obuf[0] = ch;
        setp(_obuf, _obuf + 1, _obuf + sizeof _obuf);

        return traits_type::to_int_type(ch);
    }

    int_type underflow() override
    {
        char buf[base64::encoded_size(sizeof _ibuf)];
        auto n_read = _src->sgetn(buf, sizeof buf);
        if (n_read % 4 != 0) { return traits_type::eof(); }  // errnous situation

        auto n_decoded = base64::decoded_size(array_view(buf, n_read));
        base64::decode_bytes(buf, n_read, _ibuf);

        setg(_ibuf, _ibuf, _ibuf + n_decoded);
        return traits_type::to_int_type(_ibuf[0]);
    }

    int sync() override
    {
        if (not _src) { throw std::runtime_error{"source buffer not set"}; }
        if (_n_obuf() > 0)
        {
            _write_word();
            setp(_obuf, _obuf + sizeof _obuf);
        }

        return _src->pubsync();
    }

   private:
    std::streamsize _n_obuf() const noexcept
    {
        return pptr() - pbase();
    }

    void _write_word()
    {
        char encoded[base64::encoded_size(sizeof _obuf)];

        auto n_obuf    = _n_obuf();
        auto n_written = base64::encoded_size(n_obuf);
        base64::encode_bytes(_obuf, n_obuf, encoded);
        _src->sputn(encoded, n_written);
    }
};

}  // namespace CPPHEADERS_NS_::streambuf
