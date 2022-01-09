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
#include "../helper/exception.hxx"
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
CPPH_DECLARE_EXCEPTION(archive_exception, basic_exception<archive_exception>);

CPPH_DECLARE_EXCEPTION(writer_exception, archive_exception);

CPPH_DECLARE_EXCEPTION(reader_exception, archive_exception);
CPPH_DECLARE_EXCEPTION(finished_sequence, reader_exception);
CPPH_DECLARE_EXCEPTION(invalid_context, reader_exception);
CPPH_DECLARE_EXCEPTION(parse_failed, reader_exception);
CPPH_DECLARE_EXCEPTION(read_stream_error, reader_exception);

struct key_missing : reader_exception
{
    explicit key_missing(const std::string& missing_key) : missing_key(missing_key) {}
    std::string missing_key;
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

/**
 * Error info
 */
class error_info
{
   public:
    bool has_error = false;

    int line   = 0;
    int column = 0;

    std::string message;

   private:
    friend class if_writer;
    friend class if_reader;
    uint64_t _byte_pos = 0;

   public:
    std::string str() const noexcept
    {
        std::string rval;
        rval.reserve(32 + message.size());

        auto fmt  = "line %d, column %d (B_%lld)";
        auto size = snprintf(NULL, 0, fmt, line, column, _byte_pos);
        rval.resize(size + 1);
        snprintf(rval.data(), rval.size() + 1, fmt, line, column, _byte_pos);
        rval.pop_back();

        rval += message;
        return rval;
    }

    uint64_t byte_pos() const noexcept { return _byte_pos; }
};

class if_archive_base
{
   protected:
    error_info _err = {};

   public:
    virtual ~if_archive_base() = default;

    //! Dumps additional debug information related current context
    //! (e.g. cursor pos, current token, ...)
    error_info dump_error()
    {
        auto copy = _err;
        fill_error_info(copy);
        return copy;
    }

   protected:
    virtual void fill_error_info(error_info&) const {};  // default do nothing
};

/**
 * Stream writer
 */
class if_writer : public if_archive_base
{
   private:
    stream_writer _wr;

   public:
    explicit if_writer(stream_writer writer) noexcept : _wr{std::move(writer)} {}
    ~if_writer() override = default;

   protected:
    size_t write(array_view<char const> data)
    {
        size_t n_written = _wr(data);
        _err._byte_pos += n_written;
        return n_written;
    }

   public:
    /**
     * Clear internal buffer state
     */
    void clear() { _err = {}; }

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
    virtual if_writer& operator<<(binary_t const& v)  = 0;

    virtual if_writer& object_push() = 0;
    virtual if_writer& object_pop()  = 0;

    virtual if_writer& array_push() = 0;
    virtual if_writer& array_pop()  = 0;

    //! Check if next element will be archived as key.
    virtual bool is_key_next() const = 0;
};

/**
 * Stream reader
 */
class if_reader : public if_archive_base
{
   private:
    stream_reader _rd;

   public:
    explicit if_reader(stream_reader rd) noexcept : _rd(std::move(rd)) {}
    ~if_reader() override = default;

   protected:
    size_t read(array_view<char> obuf)
    {
        auto n_read = _rd(obuf);
        _err._byte_pos += n_read;
        return n_read;
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

    virtual if_reader& operator>>(uint8_t& v) { return *this >> (int8_t&)v; }
    virtual if_reader& operator>>(uint16_t& v) { return *this >> (int16_t&)v; }
    virtual if_reader& operator>>(uint32_t& v) { return *this >> (int32_t&)v; }
    virtual if_reader& operator>>(uint64_t& v) { return *this >> (int64_t&)v; }

    virtual if_reader& operator>>(float& v) { return _upcast<double>(v); }
    virtual if_reader& operator>>(double& v) = 0;

    virtual if_reader& operator>>(std::string& v) = 0;
    virtual if_reader& operator>>(binary_t& v)    = 0;

    //! @throw parse_error next token is not valid target
    virtual bool is_object_next() = 0;
    virtual bool is_array_next()  = 0;

    //! force break of current context
    //! @throw invalid_context
    virtual void object_break() = 0;
    virtual void array_break()  = 0;

    //! Check if next element will be restored as key.
    virtual bool is_key_next() const = 0;

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

    /**
     * to distinguish variable sized object's boundary.
     *
     * level  < next_level                  := children opened.       KEEP
     * level == next_level && id == next_id := hierarchy not changed. KEEP
     * level == next_level && id != next_id := switched to unrelated. BREAK
     * level >  next_level                  := switched to parent.    BREAK
     */
    virtual void hierarchy(int* level, int* id) = 0;

   public:
    //! Goto Key. must be in object key context, and key has to exist
    void goto_key(std::string_view key)
    {
        if (not try_goto_key(key))
            throw error::key_missing{std::string{key}};
    }

    //! Check should break out of this object/array context
    bool should_break(int level, int id)
    {
        int l0 = 0, l1 = 0;
        hierarchy(&l0, &l1);

        return (level == l0 && id != l1)
            || (level > l0);
    }
};

}  // namespace CPPHEADERS_NS_::archive
