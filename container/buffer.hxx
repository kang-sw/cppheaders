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
#include <cstdlib>
#include <type_traits>

#include "../array_view.hxx"
#include "__namespace__"

namespace CPPHEADERS_NS_ {
template <typename Ty_, typename = std::enable_if_t<std::is_trivial_v<Ty_>>>
class buffer
{
   private:
    using value_type = Ty_;
    using pointer = Ty_*;
    using const_pointer = Ty_ const*;
    using reference = Ty_&;
    using const_reference = Ty_ const&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using view_type = array_view<value_type>;
    using const_view_type = array_view<value_type const>;

   private:
    pointer   _buffer = nullptr;
    size_type _size = 0;

   public:
    buffer() noexcept = default;
    explicit buffer(size_type size) { resize(size); }
    buffer(const buffer& other) { _copy_from(other); }
    buffer(buffer&& other) noexcept { _move_from(std::move(other)); }

    buffer& operator=(const buffer& other) { return _copy_from(other), *this; }
    buffer& operator=(buffer&& other) noexcept { return _move_from(std::move(other)), *this; }

    ~buffer() noexcept { _try_release(); }

   public:
    view_type       view() noexcept { return {_buffer, _size}; }
    const_view_type view() const noexcept { return {_buffer, _size}; }
    pointer         data() noexcept { return _buffer; }
    const_pointer   data() const noexcept { return _buffer; }
    reference       operator[](size_type n) noexcept { return _buffer[n]; }
    const_reference operator[](size_type n) const noexcept { return _buffer[n]; }
    size_type       size() const noexcept { return _size; }
    bool            empty() const noexcept { return _size == 0; }

    void            resize(size_type new_size)
    {
        if (_size == new_size) { return; }

        if (new_size == 0) {
            _try_release();
            return;
        }

        auto buflen = sizeof(value_type) * new_size;
        _size = new_size;

        if (not _buffer)
            _buffer = (Ty_*)malloc(buflen);
        else
            _buffer = (Ty_*)realloc(_buffer, buflen);

        if (not _buffer)
            throw std::bad_alloc{};
    }

   private:
    void _copy_from(const buffer& other)
    {
        if (other.empty()) {
            _try_release();
        } else {
            resize(other._size);
            memcpy(_buffer, other._buffer, _size * sizeof(value_type));
        }
    }

    void _move_from(buffer&& other) noexcept
    {
        _try_release();

        _buffer = other._buffer;
        _size = other._size;

        other._buffer = {};
        other._size = {};
    }

    void _try_release() noexcept
    {
        if (not _buffer) { return; }

        free(_buffer);
        _buffer = {};
        _size = {};
    }
};
}  // namespace CPPHEADERS_NS_
