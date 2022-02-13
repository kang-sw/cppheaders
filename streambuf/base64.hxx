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

        n_write_bytes = n_write_words * 3,
        n_read_bytes  = n_read_words * 3,

        read_offset = (n_write_bytes + 3) / 4 * 4
    };

    char _iobuf[read_offset + n_read_bytes];

   public:
    auto& _o() { return *reinterpret_cast<std::array<char, n_write_bytes>*>(_iobuf); }
    auto& _i() { return *reinterpret_cast<std::array<char, n_read_bytes>*>(_iobuf + read_offset); }

   public:
    explicit basic_b64(std::streambuf* adapted = nullptr) noexcept
            : _src(adapted)
    {
        setp(_o().data(), _o().data() + n_write_bytes);
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

            setp(_o().data(), _o().data() + _o().size());
            pbump(1);

            _o()[0] = ch;

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
            char buf[base64::encoded_size(n_read_bytes)];
            auto n_read = _src->sgetn(buf, sizeof buf);

            if (n_read == 0 || n_read % 4 != 0)
                return traits_type::eof();

            auto n_decoded = base64::decoded_size(array_view(buf, n_read));
            base64::decode_bytes(buf, n_read, _i().begin());

            setg(_i().data(), _i().data(), _i().data() + n_decoded);
            return traits_type::to_int_type(_i()[0]);
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
                setp(_o().data(), _o().data() + _o().size());
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
            char encoded[base64::encoded_size(n_write_bytes)];

            auto n_obuf    = _n_obuf();
            auto n_written = base64::encoded_size(n_obuf);
            base64::encode_bytes(_o().data(), n_obuf, encoded);
            _src->sputn(encoded, n_written);
        }
    }
};

using b64   = basic_b64<8, 8>;
using b64_r = basic_b64<0, 16>;
using b64_w = basic_b64<16, 0>;

}  // namespace CPPHEADERS_NS_::streambuf
