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

#include "../algorithm/std.hxx"
#include "if_archive.hxx"

//
#include "../__namespace__.h"

/**
 * Defines SAX-like interface for parsing / archiving
 */
namespace CPPHEADERS_NS_::archive {

template <typename StringLike_,
          typename std::enable_if_t<
                  not std::is_pointer_v<remove_cvr_t<StringLike_>>  //
                  && sizeof(typename StringLike_::value_type) == 1  //
                  >* = nullptr>
stream_writer obuffer(StringLike_& out)
{
    return [&out](array_view<const char> obuf) {
        out.insert(out.end(), obuf.begin(), obuf.end());
        return obuf.size();
    };
}

inline stream_writer obuffer(std::string* arg)
{
    return [arg](array_view<const char> obuf) {
        arg->append(obuf.begin(), obuf.end());
        return obuf.size();
    };
}

template <typename ViewType_>
stream_reader ibuffer(ViewType_&& view)
{
    auto arg = make_view(view);

    return [arg](array_view<char> ibuf) mutable {
        if (arg.empty()) { return eof; }

        size_t to_read = std::min(arg.size(), ibuf.size());
        copy(arg.subspan(0, to_read), ibuf.begin());
        arg = arg.subspan(to_read);
        return to_read;
    };
}

inline stream_reader ibuffer(void const* data, size_t len)
{
    return ibuffer(std::string_view{(char const*)data, len});
}

inline stream_writer obuffer(std::ostream& arg)
{
    return [&arg](array_view<const char> obuf) {
        if (not arg || arg.eof()) { return eof; }

        arg.write(obuf.data(), obuf.size());
        return obuf.size();
    };
}

inline stream_reader ibuffer(std::istream& arg)
{
    return [&arg](array_view<char> ibuf) {
        if (not arg || arg.eof()) { return int64_t(eof); }
        return arg.readsome(ibuf.data(), ibuf.size());
    };
}

}  // namespace CPPHEADERS_NS_::archive