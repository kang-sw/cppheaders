#pragma once
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <memory>

//
#include "__namespace__.h"

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
inline char const* usprintf(char const* fmt, ...)
{
    thread_local static char buf[512] = {};

    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, sizeof buf - 1, fmt, va);
    va_end(va);

    return buf;
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

}  // namespace CPPHEADERS_NS_::futils