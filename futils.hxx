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
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>

//
#include "__namespace__"

#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

namespace CPPHEADERS_NS_::futils {
namespace detail {
struct _freleae
{
    using pointer = FILE*;
    void operator()(pointer p) const noexcept { ::fclose(p); }
};
}  // namespace detail

/**
 * unsafe printf which returns thread-local storage
 */
template <typename... Args_>
inline char const* usprintf(char const* fmt, Args_&&... args)
{
    thread_local static char buf[512] = {};
    snprintf(buf, sizeof buf - 1, fmt, args...);

    return buf;
}

template <typename... Args_>
std::string ssprintf(char const* fmt, Args_&&... args)
{
    std::string str;
    str.resize((size_t)snprintf(nullptr, 0, fmt, args...) + 1);
    snprintf(str.data(), str.size(), fmt, args...);
    str.pop_back();

    return str;
}

struct file_not_exist : std::exception
{
    char const* path;
    explicit file_not_exist(char const* ptr) : path(ptr) {}
    const char* what() const noexcept override { return usprintf("file not found: %s", path); }
};

struct file_read_error : std::exception
{
    explicit file_read_error(const char* filename) : filename(filename) {}
    char const* filename;

    const char* what() const noexcept override
    {
        return usprintf("failed to read file: %s", filename);
    }
};

using file_ptr = std::unique_ptr<FILE, detail::_freleae>;

inline auto readin(char const* path)
{
    file_ptr ptr;

    ptr.reset(fopen(path, "r"));
    if (not ptr)
        throw file_not_exist{path};

    auto size = (fseek(&*ptr, 0, SEEK_END), ftell(&*ptr));
    if (size == 0)
        while (fgetc(&*ptr) != EOF) { ++size; }

    rewind(&*ptr);

    auto buffer = std::make_unique<char[]>(size);
    auto n_read = fread(buffer.get(), 1, size, &*ptr);

    if (n_read == 0)
        throw file_read_error{path};

    return std::make_pair(std::move(buffer), size);
}

inline auto readin_str(char const* path)
{
    file_ptr ptr;

    ptr.reset(fopen(path, "r"));
    if (not ptr)
        throw file_not_exist{path};

    auto size = (fseek(&*ptr, 0, SEEK_END), ftell(&*ptr));
    if (size == 0)
        while (fgetc(&*ptr) != EOF) { ++size; }

    rewind(&*ptr);

    std::string buffer;
    buffer.resize(size);
    auto n_read = fread(buffer.data(), 1, size, &*ptr);

    if (n_read == 0)
        throw file_read_error{path};

    return buffer;
}

}  // namespace CPPHEADERS_NS_::futils