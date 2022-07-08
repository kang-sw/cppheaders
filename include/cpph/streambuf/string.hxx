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
#include <string>

namespace cpph::streambuf {

/**
 * Uses existing string reference as target buffer.
 *
 * As this pre-allocate string before
 */
class stringbuf : public std::streambuf
{
    std::string _default_buf;
    std::string* _buf = &_default_buf;
    size_t _read_end = 0;

   public:
    //! Construct stringbuf
    explicit stringbuf(std::string* buf = nullptr) noexcept
    {
        if (buf) { reset(buf); }
    }

    //! Reset target buffer
    void reset(std::string* buf = nullptr)
    {
        if (_buf == &_default_buf && buf != nullptr) { _default_buf.clear(), _default_buf.shrink_to_fit(); }
        if (buf == nullptr) { buf = &_default_buf; }
        _buf = buf;
        _read_end = buf->size();  // Set initial content as read target

        setg(nullptr, nullptr, nullptr);
        _retarget();
    }

    //! Make all buffer as write target
    void clear()
    {
        _read_end = 0;

        setg(nullptr, nullptr, nullptr);
        _retarget();
    }

    //! Reserve buffer
    void reserve(size_t n)
    {
        _buf->reserve(n);
        _retarget();
    }

    //! Retrieve string-view
    std::string_view strview() const noexcept
    {
        return {_buf->data(), _read_end};
    }

    //! Retrieve string. Automatically synchronizes contents
    std::string& str() noexcept
    {
        pubsync();
        return *_buf;
    }

   private:
    void _retarget()
    {
        // Reset write buffer
        setp(_buf->data() + _read_end, _buf->data() + _buf->size());

        auto offset = gptr() - eback();
        setg(_buf->data(), _buf->data() + offset, _buf->data() + _read_end);
    }

   protected:
    int_type overflow(int_type value) override
    {
        _read_end = _buf->size();
        _buf->resize(_read_end + 228);

        _retarget();

        (*_buf)[_read_end] = traits_type::to_char_type(value);
        pbump(1);

        return value;
    }

    int_type underflow() override
    {
        auto offset = gptr() - eback();
        setg(_buf->data(), _buf->data() + offset, _buf->data() + _read_end);
        return _buf->size() == offset ? traits_type::eof() : (*_buf)[offset];
    }

    int sync() override
    {
        assert(_read_end <= _buf->size());

        _read_end += pptr() - pbase();
        _buf->resize(_read_end);

        setp(_buf->data() + _read_end, _buf->data() + _buf->size());
        stringbuf::underflow();

        return basic_streambuf::sync();
    }
};

class stringbuf_2 : public stringbuf
{
    string _str;

   public:
    stringbuf_2() noexcept : stringbuf(&_str) {}

   public:
    void reset() noexcept { stringbuf::reset(&_str); }
};

}  // namespace cpph::streambuf