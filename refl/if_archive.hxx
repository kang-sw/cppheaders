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
#include <functional>
#include <stdexcept>
#include <string_view>

#include "../functional.hxx"
#include "../template_utils.hxx"

//
#include "../__namespace__.h"

/**
 * Defines SAX-like interface for parsing / archiving
 */
namespace CPPHEADERS_NS_::archive {

/**
 * Common base class for archive errors
 */
struct archive_error : std::exception
{
};

/**
 * Binary class
 */
class binary_t : public std::vector<char>
{
   public:
    using std::vector<char>::vector;

   public:
    template <typename Ty_, std::enable_if_t<std::is_trivial_v<Ty_>>* = nullptr>
    binary_t& append(Ty_ const& value) noexcept
    {
        auto begin = std::reinterpret_pointer_cast<char const*>(&value);
        auto end   = begin + sizeof(Ty_);

        insert(begin, end);
        return *this;
    }

    template <typename Ty_, std::enable_if_t<std::is_trivial_v<Ty_>>* = nullptr>
    Ty_& refer(size_t offset_bytes = 0)
    {
        if (offset_bytes + sizeof(Ty_) > size())
            throw std::out_of_range{""};

        return *(Ty_*)data();
    }

    std::string_view view() const noexcept
    {
        return std::string_view{data(), size()};
    }

    std::string_view view(size_t offset, size_t size = ~size_t{})
    {
        size = std::min(size, this->size() - offset);

        if (offset > this->size() || offset + size > this->size())  // prevent error from underflow
            throw std::out_of_range{""};

        return std::string_view{data() + offset, size};
    }
};

/**
 * Write function requirements
 *  - returns number of bytes written successfully
 *  - throws write_error if stream status is erroneous
 */
using stream_writer = perfkit::function<size_t(std::string_view ibuf)>;

/**
 * Read function requirements
 *    - returns number of bytes written successfully.
 *    - returns 0 if there's no more data available.
 *    - throws read_error if stream status is erroneous
 */
using stream_reader = perfkit::function<int64_t(binary_t* obuf, size_t num_read)>;

/**
 *
 */
struct write_error : archive_error
{
};

/**
 * Stream writer
 */
class if_writer
{
   private:
    stream_writer _wr;

   public:
    explicit if_writer(stream_writer writer) noexcept : _wr{std::move(writer)} {}
    virtual ~if_writer() = default;

   protected:
    size_t write(std::string_view data) { return _wr(data); }

   public:
    virtual if_writer& operator<<(nullptr_t) = 0;

    virtual if_writer& operator<<(bool v) { return *this << (int64_t)v; }

    virtual if_writer& operator<<(int8_t v) { return *this << (int64_t)v; }
    virtual if_writer& operator<<(int16_t v) { return *this << (int64_t)v; }
    virtual if_writer& operator<<(int32_t v) { return *this << (int64_t)v; }
    virtual if_writer& operator<<(int64_t v) = 0;

    virtual if_writer& operator<<(uint8_t v) { return *this << (int8_t)v; }
    virtual if_writer& operator<<(uint16_t v) { return *this << (int16_t)v; }
    virtual if_writer& operator<<(uint32_t v) { return *this << (int32_t)v; }
    virtual if_writer& operator<<(uint64_t v) { return *this << (int64_t)v; }

    virtual if_writer& operator<<(float v) { return *this << (double)v; }
    virtual if_writer& operator<<(double v) = 0;

    virtual if_writer& operator<<(std::string const& v) = 0;
    virtual if_writer& operator<<(binary_t const& v)    = 0;

    virtual if_writer& object_push() = 0;
    virtual if_writer& object_pop()  = 0;

    virtual if_writer& tuple_push() = 0;
    virtual if_writer& tuple_pop()  = 0;
};

/**
 * Generic exceptions
 */
struct read_error : archive_error
{
};

struct key_missing_error : read_error
{
};

/**
 * Stream reader
 */
class if_reader
{
   private:
    stream_reader _rd;

   public:
    explicit if_reader(stream_reader rd) noexcept : _rd(std::move(rd)) {}
    virtual ~if_reader() = default;

   protected:
    virtual size_t read(binary_t* obuf, size_t num_read);

   private:
    template <typename Target_, typename Ty_>
    if_reader& _upcast(Ty_& out)
    {
        Target_ value;
        *this >> value;
        out = value;
        return *this;
    }

   public:
    virtual if_reader& operator>>(nullptr_t) = 0;

    virtual if_reader& operator>>(bool& v) { return _upcast<int64_t>(v); }

    virtual if_reader& operator>>(int8_t& v) { return _upcast<int64_t>(v); }
    virtual if_reader& operator>>(int16_t& v) { return _upcast<int64_t>(v); }
    virtual if_reader& operator>>(int32_t& v) { return _upcast<int64_t>(v); }
    virtual if_reader& operator>>(int64_t& v) = 0;

    virtual if_reader& operator>>(float& v) { return _upcast<double>(v); }
    virtual if_reader& operator>>(double& v) = 0;

    virtual if_reader& operator>>(std::string& v) = 0;
    virtual if_reader& operator>>(binary_t& v)    = 0;

    virtual bool goto_key(std::string_view key) = 0;
    virtual bool tuple_has_next()               = 0;

   public:
    void require_key(std::string_view key)
    {
        if (not goto_key(key))
            throw key_missing_error{};
    }
};

}  // namespace CPPHEADERS_NS_::archive
