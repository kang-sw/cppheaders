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

#include "../../__namespace__"
#include "../../array_view.hxx"
#include "../../functional.hxx"
#include "../../helper/exception.hxx"
#include "../../template_utils.hxx"

/**
 * Defines SAX-like interface for parsing / archiving
 */
namespace CPPHEADERS_NS_::archive {

class if_writer;
class if_reader;

namespace error {

/**
 * Common base class for archive errors
 */
CPPH_DECLARE_EXCEPTION(archive_exception, basic_exception<archive_exception>);

struct writer_exception : archive_exception
{
    if_writer* writer;
    explicit writer_exception(if_writer* wr) : writer(wr) {}
};

CPPH_DECLARE_EXCEPTION(writer_invalid_context, writer_exception);
CPPH_DECLARE_EXCEPTION(writer_invalid_state, writer_exception);

struct reader_exception : archive_exception
{
    if_reader* reader;
    explicit reader_exception(if_reader* rd) : reader(rd) {}
};

CPPH_DECLARE_EXCEPTION(reader_finished_sequence, reader_exception);
CPPH_DECLARE_EXCEPTION(reader_invalid_context, reader_exception);
CPPH_DECLARE_EXCEPTION(reader_parse_failed, reader_exception);
CPPH_DECLARE_EXCEPTION(reader_read_stream_error, reader_exception);

struct reader_key_missing : reader_exception
{
    explicit reader_key_missing(
            if_reader* rd,
            const std::string& missing_key)
            : reader_exception(rd),
              missing_key(missing_key) {}

    std::string missing_key;
};

}  // namespace error

constexpr size_t eof = ~size_t{};

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
    explicit if_writer(stream_writer ostrm) noexcept : _wr{std::move(ostrm)} {}
    ~if_writer() override = default;

   protected:
    size_t write_str(std::string_view data)
    {
        return write(data);
    }

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
    template <typename ValTy_>
    if_writer& serialize(ValTy_ const&);

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

    //! writes binary_t as view form
    virtual if_writer& write_binary(const_buffer_view v) = 0;

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
    explicit if_reader(stream_reader istrm) noexcept : _rd(std::move(istrm)) {}
    ~if_reader() override = default;

   protected:
    size_t read(array_view<char> obuf)
    {
        auto n_read = _rd(obuf);
        _err._byte_pos += n_read;
        return n_read;
    }

   private:
    template <typename Target_, typename ValTy_>
    if_reader& _upcast(ValTy_& out)
    {
        Target_ value;
        *this >> value;
        out = static_cast<ValTy_>(value);
        return *this;
    }

   public:
    template <typename ValTy_>
    if_reader& deserialize(ValTy_& out);

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

    virtual if_reader& operator>>(std::string& v)            = 0;
    virtual if_reader& read_binary(mutable_buffer_view obuf) = 0;

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
