// MIT License
//
// Copyright (c) 2022. Seungwoo Kang
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
#include <iostream>

#include "if_archive.hxx"

//
#include "../__namespace__.h"

/**
 * Defines SAX-like interface for parsing / archiving
 */
namespace CPPHEADERS_NS_::archive::stream {

stream_writer ostring(std::string* arg)
{
    return [arg](array_view<const char> obuf) {
        arg->append(obuf.begin(), obuf.end());
        return obuf.size();
    };
}

stream_reader istring(std::string_view arg)
{
    return [arg](array_view<char> ibuf) mutable {
        if (arg.empty()) { return eof; }

        size_t to_read = std::min(arg.size(), ibuf.size());
        copy(arg.substr(0, to_read), ibuf.begin());
        arg = arg.substr(to_read);
        return to_read;
    };
}

stream_writer ostream(std::ostream& arg)
{
    return [&arg](array_view<const char> obuf) {
        if (not arg || arg.eof()) { return eof; }

        arg.write(obuf.data(), obuf.size());
        return obuf.size();
    };
}

stream_reader istream(std::istream& arg)
{
    return [&arg](array_view<char> ibuf) {
        if (not arg || arg.eof()) { return int64_t(eof); }
        return arg.readsome(ibuf.data(), ibuf.size());
    };
}

}  // namespace CPPHEADERS_NS_::archive::stream