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

#include "../array_view.hxx"
#include "../functional.hxx"
#include "../template_utils.hxx"

//
#include "../__namespace__.h"

/**
 * Defines SAX-like interface for parsing / archiving
 */
namespace CPPHEADERS_NS_::archive {

namespace error {

/**
 * Common base class for archive errors
 */
struct archive_exception : std::exception
{
   private:
    mutable std::string _message;

   public:
    archive_exception() = default;

    explicit archive_exception(std::string message) noexcept
            : _message(message) {}

    archive_exception message(std::string_view content)
    {
        _setmsg(content);
        return std::move(*this);
    }

    template <typename Str_, typename Arg0_, typename... Args_>
    archive_exception message(Str_&& str, Arg0_&& arg0, Args_&&... args)
    {
        auto buflen = snprintf(nullptr, 0, str, arg0, args...);
        _message.resize(buflen + 1);

        snprintf(_message.data(), _message.size(), str, arg0, args...);
        _message.pop_back();  // remove null character

        return std::move(*this);
    }

    const char* what() const override
    {
        if (_message.empty()) { _setmsg(typeid(*this).name()); }
        return _message.c_str();
    }

   private:
    void _setmsg(std::string_view content) const
    {
        _message = "archive error: ";
        _message += content;
    }
};

}  // namespace error

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
using stream_writer = function<size_t(array_view<char const> ibuf)>;

/**
 * Read function requirements
 *    - returns number of bytes written successfully.
 *    - returns 0 if there's no more data available.
 *    - throws read_error if stream status is erroneous
 */
using stream_reader = function<int64_t(array_view<char> obuf)>;

namespace error {
/**
 *
 */
struct writer_exception : archive_exception
{
};
}  // namespace error

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
    size_t write(array_view<char const> data) { return _wr(data); }

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

    virtual if_writer& operator<<(std::string_view v) = 0;
    virtual if_writer& operator<<(binary_t const& v)    = 0;

    virtual if_writer& object_push() = 0;
    virtual if_writer& object_pop()  = 0;

    virtual if_writer& array_push() = 0;
    virtual if_writer& array_pop()  = 0;

    //! Check if next element will be archived as key.
    virtual bool is_key_next() const = 0;
};

namespace error {
/**
 * Generic exceptions
 */
struct reader_exception : archive_exception
{
};

struct finished_sequence : reader_exception
{
};

struct invalid_context : reader_exception
{
};

struct parse_failed : reader_exception
{
};

struct key_missing : parse_failed
{
    std::string key;
};

struct read_stream_error : reader_exception
{
};
}  // namespace error

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
    size_t read(array_view<char> obuf)
    {
        return _rd(obuf);
    }

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

    //! @throw parse_error next token is not valid target
    virtual void object_enter() = 0;
    virtual void array_enter()  = 0;

    //! @throw invalid_request if current context is not given sequence
    virtual void object_exit() = 0;
    virtual void array_exit()  = 0;

    //! check if next statement is null
    virtual bool is_null_next() const = 0;

    //!
    virtual bool eof() = 0;

    /**
     * Move to key and join.
     *
     * @throw invalid_request if current context is not key sequence
     */
    virtual bool try_goto_key(std::string_view key) = 0;

    //! Check if next element will be restored as key.
    virtual bool is_key_next() const = 0;

    //!

    /**
     * to distinguish variable sized object's boundary.
     *
     * level  < next_level                  := children opened.       KEEP
     * level == next_level && id == next_id := hierarchy not changed. KEEP
     * level == next_level && id != next_id := switched to unrelated. BREAK
     * level >  next_level                  := switched to parent.    BREAK
     */
    virtual void next_hierarchy(int* level, int* id) = 0;

   public:
    /**
     * Goto Key. must be in object key context, and key has to exist
     */
    void goto_key(std::string_view key)
    {
        if (not try_goto_key(key))
            throw error::key_missing{}.message(key);
    }
};

}  // namespace CPPHEADERS_NS_::archive
