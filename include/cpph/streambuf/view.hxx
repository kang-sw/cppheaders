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

#include "../utility/array_view.hxx"

namespace cpph::streambuf {
class view : public std::streambuf
{
   public:
    explicit view(array_view<char> buf = {})
    {
        reset(buf);
    }

    void reset(array_view<char> buf)
    {
        setg(buf.data(), buf.data(), buf.data() + buf.size());
        setp(buf.data(), buf.data() + buf.size());
    }
};

class const_view : public std::streambuf
{
   public:
    explicit const_view(const_buffer_view buf = {})
    {
        reset(buf);
    }

    void reset(const_buffer_view buf)
    {
        auto data = const_cast<char*>(buf.data());
        setg(data, data, data + buf.size());
    }
};

}  // namespace cpph::streambuf