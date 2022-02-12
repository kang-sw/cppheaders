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
#include <stdexcept>
#include <streambuf>
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

/**
 * List of available property formats
 */
enum class entity_type : uint16_t
{
    invalid,

    object,
    dictionary,

    tuple,
    array,

    binary,

    null,
    boolean,
    integer,
    floating_point,
    string,
};

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
    explicit writer_exception(if_writer const* wr) : writer((if_writer*)wr) {}
};

CPPH_DECLARE_EXCEPTION(writer_invalid_state, writer_exception);

struct reader_exception : archive_exception
{
    if_reader* reader;
    explicit reader_exception(if_reader const* rd) : reader((if_reader*)rd) {}
};

CPPH_DECLARE_EXCEPTION(reader_finished_sequence, reader_exception);
CPPH_DECLARE_EXCEPTION(reader_invalid_context, reader_exception);
CPPH_DECLARE_EXCEPTION(reader_parse_failed, reader_exception);
CPPH_DECLARE_EXCEPTION(reader_read_stream_error, reader_exception);

CPPH_DECLARE_EXCEPTION(reader_assertion_failed, reader_exception);

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
    error_info _err      = {};
    std::streambuf* _buf = {};

   public:
    bool use_integer_key : 1;

   public:
    virtual ~if_archive_base() = default;
    explicit if_archive_base(std::streambuf* buf) noexcept
            : _buf(buf),
              use_integer_key(false)
    {
    }

    //! Gets internal buffer
    std::streambuf* rdbuf() const noexcept { return _buf; }

    std::streambuf* rdbuf(std::streambuf* buf) noexcept { return std::swap(buf, _buf), buf; }

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
   public:
    explicit if_writer(std::streambuf* buf) noexcept : if_archive_base(buf) {}
    ~if_writer() override = default;

   public:
    /**
     * Clear internal buffer state
     */
    void clear() { _err = {}; }

   public:
    template <typename ValTy_>
    if_writer& serialize(ValTy_ const&);

   public:
    virtual if_writer& write(nullptr_t) = 0;

    virtual if_writer& write(bool v) { return this->write((int64_t)v); }

    virtual if_writer& write(int8_t v) { return this->write((int64_t)v); }
    virtual if_writer& write(int16_t v) { return this->write((int64_t)v); }
    virtual if_writer& write(int32_t v) { return this->write((int64_t)v); }
    virtual if_writer& write(int64_t v) = 0;

    virtual if_writer& write(uint8_t v) { return this->write((int64_t)v); }
    virtual if_writer& write(uint16_t v) { return this->write((int64_t)v); }
    virtual if_writer& write(uint32_t v) { return this->write((int64_t)v); }
    virtual if_writer& write(uint64_t v) { return this->write((int64_t)v); }

    virtual if_writer& write(float v) { return this->write((double)v); }
    virtual if_writer& write(double v) = 0;

    virtual if_writer& write(std::string_view v) = 0;
    if_writer& write(std::string const& v) { return this->write(std::string_view{v}); }

    if_writer& write(const_buffer_view v)
    {
        binary_push(v.size());
        binary_write_some(v);
        binary_pop();
        return *this;
    }

    //! Serialize arbitrary type
    template <typename Ty_>
    if_writer& write(Ty_ const& other);

    //! push/pop write binary context
    //! Firstly pushed binary size is immutable. call binary_pop only when
    //!  'total' bytes was written!
    virtual if_writer& binary_push(size_t total)            = 0;
    virtual if_writer& binary_write_some(const_buffer_view) = 0;
    virtual if_writer& binary_pop()                         = 0;

    //! push objects which has 'num_elems' elements
    //! set -1 when unkown. (maybe rare case)
    virtual if_writer& object_push(size_t num_elems) = 0;
    virtual if_writer& object_pop()                  = 0;

    //! push array which has 'num_elems' elements
    virtual if_writer& array_push(size_t num_elems) = 0;
    virtual if_writer& array_pop()                  = 0;

    //! Assert to write key next
    virtual void write_key_next() = 0;
};

/**
 * A context key to represent current parsing context
 */
struct context_key
{
    int64_t value;
};

/**
 * Stream reader
 */
class if_reader : public if_archive_base
{
   public:
    explicit if_reader(std::streambuf* buf) noexcept : if_archive_base(buf) {}
    ~if_reader() override = default;

   private:
    template <typename Target_, typename ValTy_>
    if_reader& _upcast(ValTy_& out)
    {
        Target_ value;
        this->read(value);
        out = static_cast<ValTy_>(value);
        return *this;
    }

   public:
    template <typename ValTy_>
    if_reader& deserialize(ValTy_& out);

   public:
    virtual if_reader& read(nullptr_t) = 0;

    virtual if_reader& read(bool& v) { return _upcast<int64_t>(v); }

    virtual if_reader& read(int8_t& v) { return _upcast<int64_t>(v); }
    virtual if_reader& read(int16_t& v) { return _upcast<int64_t>(v); }
    virtual if_reader& read(int32_t& v) { return _upcast<int64_t>(v); }
    virtual if_reader& read(int64_t& v) = 0;

    virtual if_reader& read(uint8_t& v) { return this->read((int8_t&)v); }
    virtual if_reader& read(uint16_t& v) { return this->read((int16_t&)v); }
    virtual if_reader& read(uint32_t& v) { return this->read((int32_t&)v); }
    virtual if_reader& read(uint64_t& v) { return this->read((int64_t&)v); }

    virtual if_reader& read(float& v) { return _upcast<double>(v); }
    virtual if_reader& read(double& v) = 0;

    virtual if_reader& read(std::string& v) = 0;

    //! Deserialize arbitrary type
    template <typename Ty_>
    if_reader& read(Ty_& other);

    //! Tries to get number of remaining element for currently active context.
    //! @return -1 if feature not available
    virtual size_t elem_left() const { return ~size_t{}; }

    //! returns next binary size
    //! @throw parse_error if current context is not binary (binary can read multiple times)
    //! @return number of bytes to be read from stream. If binary size cannot be known without
    //!          reading content, implementation may return -1.
    virtual size_t begin_binary() = 0;

    //! @return number of bytes actually read.
    virtual size_t binary_read_some(mutable_buffer_view v) = 0;
    virtual void end_binary()                              = 0;

    /**
     * to distinguish variable sized object's boundary.
     *
     * depth  < next_depth                  := children opened.       KEEP
     * depth == next_depth && id == next_id := hierarchy not changed. KEEP
     * depth == next_depth && id != next_id := switched to unrelated. BREAK
     * depth >  next_depth                  := switched to parent.    BREAK
     */

    //! enter into context
    //! @throw invalid_context if
    virtual context_key begin_object() = 0;
    virtual context_key begin_array()  = 0;

    //! Check should break out of this object/array context
    //! Used for dynamic object retrival
    virtual bool should_break(context_key const& key) const = 0;

    //! break current context, regardless if there's any remaining object.
    //! @throw invalid_context
    virtual void end_object(context_key) = 0;
    virtual void end_array(context_key)  = 0;

    //! Assert key on next read
    virtual void read_key_next() = 0;

    //! check if next statement is null
    virtual bool is_null_next() const = 0;

    //! type getters
    virtual bool is_object_next() const = 0;
    virtual bool is_array_next() const  = 0;
};

template <typename Any_>
if_writer& operator<<(if_writer& writer, Any_ const& value)
{
    return writer.write(value);
}

template <typename Any_>
if_reader& operator>>(if_reader& reader, Any_& ref)
{
    return reader.read(ref);
}

}  // namespace CPPHEADERS_NS_::archive
