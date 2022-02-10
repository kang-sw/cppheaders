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
class basic_b64 : public std::streambuf
{
    std::streambuf* _src;

    enum
    {
        n_write_words = WriteWords_,
        n_read_words  = ReadWords_,
    };

    char _iobuf[n_write_words * 3 + n_read_words * 3];

   public:
    auto& _obuf() { return (char(&)[n_write_words * 3]) _iobuf; }
    auto& _ibuf() { return (char(&)[n_read_words * 3]) _iobuf[n_write_words * 3]; }

   public:
    explicit basic_b64(std::streambuf* adapted = nullptr) noexcept
            : _src(adapted)
    {
        setp(_obuf(), _obuf() + n_write_words * 3);
    }

    std::streambuf* reset(std::streambuf* adapted = nullptr) noexcept
    {
        if (_src) { sync(); }
        return std::swap(_src, adapted), adapted;
    }

    ~basic_b64() noexcept override
    {
        if (_src && _n_obuf() > 0) { _write_word(); }
    }

   protected:
    int_type overflow(int_type ch) override
    {
        if constexpr (n_write_words > 0)
        {
            if (_n_obuf() != 0) { _write_word(); }

            _obuf()[0] = ch;
            setp(_obuf(), _obuf() + 1, _obuf() + sizeof _obuf());

            return traits_type::to_int_type(ch);
        }
        else
        {
            return traits_type::eof();
        }
    }

    int_type underflow() override
    {
        if constexpr (n_read_words > 0)
        {
            char buf[base64::encoded_size(sizeof _ibuf())];
            auto n_read = _src->sgetn(buf, sizeof buf);
            if (n_read % 4 != 0) { return traits_type::eof(); }  // errnous situation

            auto n_decoded = base64::decoded_size(array_view(buf, n_read));
            base64::decode_bytes(buf, n_read, _ibuf());

            setg(_ibuf(), _ibuf(), _ibuf() + n_decoded);
            return traits_type::to_int_type(_ibuf()[0]);
        }
        else
        {
            return traits_type::eof();
        }
    }

    int sync() override
    {
        if (not _src) { throw std::runtime_error{"source buffer not set"}; }

        if constexpr (n_write_words > 0)
            if (_n_obuf() > 0)
            {
                _write_word();
                setp(_obuf(), _obuf() + sizeof _obuf());
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
        if constexpr (n_write_words > 0)
        {
            char encoded[base64::encoded_size(sizeof _obuf())];

            auto n_obuf    = _n_obuf();
            auto n_written = base64::encoded_size(n_obuf);
            base64::encode_bytes(_obuf(), n_obuf, encoded);
            _src->sputn(encoded, n_written);
        }
    }
};

using b64   = basic_b64<8, 8>;
using b64_r = basic_b64<0, 16>;
using b64_w = basic_b64<16, 0>;

}  // namespace CPPHEADERS_NS_::streambuf
