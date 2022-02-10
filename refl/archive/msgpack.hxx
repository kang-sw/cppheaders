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

#include "detail/context_helper.hxx"

namespace CPPHEADERS_NS_::archive::msgpack {
enum class typecode : uint8_t
{
    positive_fixint = 0b0'0000000,
    negative_fixint = 0b111'00000,

    fixmap   = 0b1000'0000,
    fixarray = 0b1001'0000,
    fixstr   = 0b101'00000,

    error = 0xc1,

    nil        = 0xc0,
    bool_false = 0xc2,
    bool_true  = 0xc3,

    bin8 = 0xc4,
    bin16,
    bin32,

    float32 = 0xca,
    float64 = 0xcb,
    uint8   = 0xcc,
    uint16  = 0xcd,
    uint32  = 0xce,
    uint64  = 0xcf,
    int8    = 0xd0,
    int16   = 0xd1,
    int32   = 0xd2,
    int64   = 0xd3,

    //! @warning EXT types are treated as simple binary. Typecodes will be ignored.
    fixext1 = 0xd4,
    fixext2,
    fixext4,
    fixext8,
    fixext16,

    ext8 = 0xc7,
    ext16,
    ext32,

    str8  = 0xd9,
    str16 = 0xda,
    str32 = 0xdb,

    array16 = 0xdc,
    array32 = 0xdd,

    map16 = 0xde,
    map32 = 0xdf,
};

class writer : public archive::if_writer
{
   private:
    detail::write_context_helper _ctx;

   private:
    template <typename ValTy_, typename Ty_, typename = std::enable_if_t<std::is_trivial_v<ValTy_>>>
    void _putbin(Ty_ const& val)
    {
        char inv[sizeof(ValTy_)];

        for (auto i = 0; i < sizeof(ValTy_); ++i)
            inv[sizeof(ValTy_) - 1 - i] = ((char const*)&val)[i];

        _buf->sputn(inv, sizeof inv);
    }

    void _ap(typecode code)
    {
        _buf->sputc(char(code));
    }

    // for str, bin, ext
    void _apsize_1(typecode code, uint32_t size)
    {
        if (size & 0xffff0000)
        {
            _apoff(code, 2);
            _putbin<uint32_t>(size);
        }
        else if (size & 0xff00)
        {
            _apoff(code, 1);
            _putbin<uint16_t>(size);
        }
        else
        {
            _apoff(code, 0);
            _putbin<uint8_t>(size);
        }
    }

    void _apoff(typecode code, int ofst)
    {
        _buf->sputc(char(code) + ofst);
    }

    template <size_t MaskBits_>
    void _apm(typecode code, uint8_t value)
    {
        static_assert(MaskBits_ < 8);
        constexpr uint8_t mask = (1u << MaskBits_) - 1;
        _buf->sputc(uint8_t(code) | value & mask);
    }

    if_writer& _write_int(int64_t value)
    {
        _ctx.write_next();

        if (value >= 0)
        {
            if (value < 0x80)
                _apm<7>(typecode::positive_fixint, value);
            else if (value < 0x8000)
                _ap(typecode::int16), _putbin<int16_t>(value);
            else if (value < 0x8000'0000)
                _ap(typecode::int32), _putbin<int32_t>(value);
            else
                _ap(typecode::int64), _putbin<int64_t>(value);
        }
        else
        {
            if (value >= -32)
                _apm<5>(typecode::negative_fixint, value);
            else if (value >= -0x8000)
                _ap(typecode::int16), _putbin<int16_t>(value);
            else if (value >= -0x8000'0000ll)
                _ap(typecode::int32), _putbin<int32_t>(value);
            else
                _ap(typecode::int64), _putbin<int64_t>(value);
        }

        return *this;
    }

    if_writer& _write_uint(uint64_t value)
    {
        _ctx.write_next();

        if (value < 0x80)
            _apm<7>(typecode::positive_fixint, value);
        else if (value < 0x8000)
            _ap(typecode::uint16), _putbin<uint16_t>(value);
        else if (value < 0x8000'0000)
            _ap(typecode::uint32), _putbin<uint32_t>(value);
        else
            _ap(typecode::uint64), _putbin<uint64_t>(value);

        return *this;
    }

    void _assert_32bitsize(size_t n)
    {
        if (n >= (std::numeric_limits<uint32_t>::max)())
        {
            throw archive::error::writer_out_of_range{this}.message("size exceeds 32bit range");
        }
    }

   public:
    explicit writer(std::streambuf* buf, size_t depth_estimated = 0)
            : archive::if_writer(buf)
    {
        _ctx.reserve_depth(depth_estimated);
    }

    void reserve_depth(size_t n) { _ctx.reserve_depth(n); }

    if_writer& write(nullptr_t a_nullptr) override
    {
        _ctx.write_next();
        _ap(typecode::nil);
        return *this;
    }

    if_writer& write(bool v) override
    {
        _ctx.write_next();
        _ap(v ? typecode::bool_true : typecode::bool_false);
        return *this;
    }

    if_writer& write(std::string_view v) override
    {
        _assert_32bitsize(v.size());

        _ctx.write_next();

        if (v.size() < 32)
            _apm<5>(typecode::fixstr, v.size());
        else
            _apsize_1(typecode::str8, v.size());

        _buf->sputn(v.data(), v.size());
        return *this;
    }

    if_writer& write(float v) override
    {
        _ctx.write_next();
        _ap(typecode::float32), _putbin<float>(v);
        return *this;
    }

    if_writer& write(double v) override
    {
        _ctx.write_next();
        _ap(typecode::float64), _putbin<double>(v);
        return *this;
    }

    if_writer& write(int8_t v) override { return _write_int(v); }
    if_writer& write(int16_t v) override { return _write_int(v); }
    if_writer& write(int32_t v) override { return _write_int(v); }
    if_writer& write(int64_t v) override { return _write_int(v); }

    if_writer& write(uint8_t v) override { return _write_uint(v); }
    if_writer& write(uint16_t v) override { return _write_uint(v); }
    if_writer& write(uint32_t v) override { return _write_uint(v); }
    if_writer& write(uint64_t v) override { return _write_uint(v); }

    if_writer& binary_push(size_t total) override
    {
        _assert_32bitsize(total);

        _ctx.write_next();
        _ctx.push_binary(total);

        _apsize_1(typecode::bin8, total);
        return *this;
    }

    if_writer& binary_write_some(const_buffer_view view) override
    {
        _ctx.binary_write_some(view.size());
        _buf->sputn(view.data(), view.size());
        return *this;
    }

    if_writer& binary_pop() override
    {
        _ctx.pop_binary();
        return *this;
    }

    if_writer& object_push(size_t num_elems) override
    {
        _assert_32bitsize(num_elems);

        _ctx.write_next();
        _ctx.push_object(num_elems);

        if (num_elems < 16)
            _apm<4>(typecode::fixmap, num_elems);
        else if (num_elems < 0x8000)
            _ap(typecode::map16), _putbin<uint16_t>(num_elems);
        else
            _ap(typecode::map32), _putbin<uint32_t>(num_elems);

        return *this;
    }

    if_writer& object_pop() override
    {
        _ctx.pop_object();
        return *this;
    }

    if_writer& array_push(size_t num_elems) override
    {
        _assert_32bitsize(num_elems);

        _ctx.write_next();
        _ctx.push_array(num_elems);

        if (num_elems < 16)
            _apm<4>(typecode::fixarray, num_elems);
        else if (num_elems < 0x8000)
            _ap(typecode::array16), _putbin<uint16_t>(num_elems);
        else
            _ap(typecode::array32), _putbin<uint32_t>(num_elems);

        return *this;
    }

    if_writer& array_pop() override
    {
        _ctx.pop_array();
        return *this;
    }

    void write_key_next() override
    {
        _ctx.write_key_next();
    }
};

class reader : public archive::if_reader
{
    union key_t
    {
        context_key data;
        struct
        {
            uint32_t id;
            uint32_t index;
        };
    };

    struct scope_t
    {
        enum type_t
        {
            type_obj,
            type_array,
            type_binary
        };

        key_t key;
        type_t type;

        uint32_t elems_left;
        bool reading_key = false;
    };

   private:
    std::vector<scope_t> _scope;
    uint32_t _scope_key_gen = 0;

   public:
    explicit reader(std::streambuf* buf, size_t reserved_depth = 0)
            : archive::if_reader(buf)
    {
        _scope.reserve(reserved_depth);
    }

    void reserve_depth(size_t n) { _scope.reserve(n); }

   private:
    template <typename ValTy_, typename CastTo_ = ValTy_>
    CastTo_ _get_n_endian()
    {
        char buffer[sizeof(ValTy_)];
        for (int i = sizeof buffer - 1; i >= 0; --i)
            buffer[i] = _buf->sbumpc();

        return static_cast<CastTo_>(*reinterpret_cast<ValTy_*>(buffer));
    }

    constexpr static typecode _do_offset(typecode value, int n)
    {
        return typecode((int)value + n);
    }

    // Used for str8, bin8
    template <typecode Base_, typecode Fix_ = typecode::error, size_t FixMask_ = 0,
              typename HeaderChar_,
              typename = std::enable_if_t<sizeof(HeaderChar_) == 1>>
    uint32_t _read_elem_count(HeaderChar_ header)
    {
        switch (typecode(header))
        {
            case Fix_: return (uint8_t)header & FixMask_;
            case Base_: return _get_n_endian<uint8_t>();
            case _do_offset(Base_, 1): return _get_n_endian<uint16_t>();
            case _do_offset(Base_, 2): return _get_n_endian<uint32_t>();

            default:
                throw error::reader_parse_failed{this}.message("type error");
        }
    }

    // Used for array16, bin16

    double _parse_number(char header)
    {
        char buf[64];
        char buflen = (char)_read_elem_count<typecode::str8, typecode::fixstr, 31>(header);

        if (buflen >= sizeof buf)
            throw error::reader_parse_failed{this}.message("too big number");

        if (_buf->sgetn(buf, buflen) != buflen)
            throw error::reader_read_stream_error{this}.message("invalid EOF");

        buf[buflen] = '\0';
        char* tail;
        auto value = strtod(buf, &tail);

        if (tail - buf != buflen)
            throw error::reader_parse_failed{this}.message("given string is not a number");

        return value;
    }

    template <typename ValTy_>
    auto _read_number(char header)
    {
        auto const code = typecode(header);
        switch (code)
        {
            case typecode::positive_fixint:
            case typecode::negative_fixint:
                return ValTy_(header);

            case typecode::bool_false: return ValTy_(0);
            case typecode::bool_true: return ValTy_(1);

            case typecode::float32: return _get_n_endian<float, ValTy_>();
            case typecode::float64: return _get_n_endian<double, ValTy_>();

            case typecode::uint8: return _get_n_endian<uint8_t, ValTy_>();
            case typecode::uint16: return _get_n_endian<uint16_t, ValTy_>();
            case typecode::uint32: return _get_n_endian<uint32_t, ValTy_>();
            case typecode::uint64: return _get_n_endian<uint64_t, ValTy_>();
            case typecode::int8: return _get_n_endian<int8_t, ValTy_>();
            case typecode::int16: return _get_n_endian<int16_t, ValTy_>();
            case typecode::int32: return _get_n_endian<int32_t, ValTy_>();
            case typecode::int64: return _get_n_endian<int64_t, ValTy_>();

            case typecode::fixstr:
            case typecode::str8:
            case typecode::str16:
            case typecode::str32:
                return ValTy_(_parse_number(header));

            default:
                throw error::reader_parse_failed{(if_reader*)this}
                        .message("number type expected: %02x", code);
        }
    }

   public:
    if_reader& read(nullptr_t) override
    {
        // skip single item
        _skip_once();
        return *this;
    }

    if_reader& read(bool& v) override
    {
        _step_context();
        v = _read_number<bool>(_verify_eof(_buf->sbumpc()));
        return *this;
    }

   private:
    template <typename T_>
    if_reader& _quick_get_num(T_& ref)
    {
        _step_context();
        ref = _read_number<T_>(_verify_eof(_buf->sbumpc()));
        return *this;
    }

   public:
    if_reader& read(int8_t& v) override { return _quick_get_num(v); }
    if_reader& read(int16_t& v) override { return _quick_get_num(v); }
    if_reader& read(int32_t& v) override { return _quick_get_num(v); }
    if_reader& read(int64_t& v) override { return _quick_get_num(v); }
    if_reader& read(uint8_t& v) override { return _quick_get_num(v); }
    if_reader& read(uint16_t& v) override { return _quick_get_num(v); }
    if_reader& read(uint32_t& v) override { return _quick_get_num(v); }
    if_reader& read(uint64_t& v) override { return _quick_get_num(v); }
    if_reader& read(float& v) override { return _quick_get_num(v); }
    if_reader& read(double& v) override { return _quick_get_num(v); }

    if_reader& read(std::string& v) override
    {
        _step_context();

        auto header     = _verify_eof(_buf->sbumpc());
        uint32_t buflen = _read_elem_count<typecode::str8, typecode::fixstr, 31>(header);

        v.resize(buflen);
        _buf->sgetn(v.data(), v.size());

        return *this;
    }

    size_t elem_left() const override { return _scope_ref().elems_left; }

    bool should_break(const context_key& key) const override
    {
        auto scope = &_scope.back();
        return key.value == scope->key.data.value && scope->elems_left == 0;
    }

    context_key begin_object() override
    {
        _verify_not_key_type();
        _step_context();

        auto header = _verify_eof(_buf->sbumpc());
        auto n_elem = _read_elem_count<_do_offset(typecode::map16, -1), typecode::fixmap, 15>(header);

        return _new_scope(scope_t::type_obj, n_elem)->key.data;
    }

    void end_object(context_key key) override
    {
        _verify_end(key);
        _break_scope();
    }

    size_t begin_binary() override
    {
        _verify_not_key_type();
        _step_context();

        auto header     = _verify_eof(_buf->sbumpc());
        uint32_t buflen = _read_elem_count<typecode::bin8>(header);

        _new_scope(scope_t::type_binary, buflen);
        return buflen;
    }

    size_t binary_read_some(mutable_buffer_view v) override;

    void end_binary() override;

    context_key begin_array() override;

    void end_array(context_key key) override;
    void read_key_next() override;

    bool is_null_next() const override;
    bool is_object_next() const override;
    bool is_array_next() const override;

   private:
    void _break_scope()
    {
        for (auto scope = &_scope_ref(); scope->elems_left > 0; --scope->elems_left)
            _skip_once();

        _scope.pop_back();
    }

    void _skip_once()
    {
        // intentionally uses getc instead of bumpc
        auto header         = _verify_eof(_buf->sgetc());
        uint32_t skip_bytes = 0;
        switch (typecode(header))
        {
            case typecode::positive_fixint:
            case typecode::negative_fixint:
            case typecode::bool_false:
            case typecode::bool_true:
            case typecode::float32:
            case typecode::float64:
            case typecode::uint8:
            case typecode::uint16:
            case typecode::uint32:
            case typecode::uint64:
            case typecode::int8:
            case typecode::int16:
            case typecode::int32:
            case typecode::int64:
                _buf->sbumpc();
                _read_number<uint64_t>(header);
                break;

            case typecode::fixstr:
            case typecode::str8:
            case typecode::str16:
            case typecode::str32:
                // 1 for header, bumpc
                skip_bytes = 1 + _read_elem_count<typecode::str8, typecode::fixstr, 31>(header);
                break;

            case typecode::bin8:
            case typecode::bin16:
            case typecode::bin32:
                skip_bytes = 1 + _read_elem_count<typecode::bin8>(header) + 1;
                break;

            case typecode::fixarray:
            case typecode::array16:
            case typecode::array32:
                end_array(begin_array());
                break;

            case typecode::fixmap:
            case typecode::map16:
            case typecode::map32:
                end_object(begin_object());
                break;

            case typecode::nil:
                skip_bytes = 1;
                break;

            case typecode::fixext1: skip_bytes = 3; break;
            case typecode::fixext2: skip_bytes = 4; break;
            case typecode::fixext4: skip_bytes = 6; break;
            case typecode::fixext8: skip_bytes = 10; break;
            case typecode::fixext16: skip_bytes = 18; break;

            case typecode::ext8:
            case typecode::ext16:
            case typecode::ext32:
                skip_bytes = 1 + _read_elem_count<typecode::ext8>(header);
                break;

            case typecode::error:
                throw error::reader_parse_failed{this}.message("unsupported format: %02x", header);
        }

        while (skip_bytes--) { _buf->sbumpc(); }
    }

    void _verify_end(context_key key)
    {
        // top scope is same as given key
    }

    scope_t* _new_scope(scope_t::type_t ty, uint32_t n_elems)
    {
        auto scope         = &_scope.emplace_back();
        scope->type        = ty;
        scope->elems_left  = n_elems + n_elems * (ty == scope_t::type_obj);
        scope->reading_key = false;
        scope->key.index   = uint32_t(_scope.size() - 1);
        scope->key.id      = ++_scope_key_gen;

        return scope;
    }

    void _verify_not_key_type()
    {
        if (_scope.empty()) { return; }

        auto const& scope = _scope.back();
        if (scope.type != scope_t::type_obj
            || (scope.elems_left & 1) && not scope.reading_key)
        {
            return;
        }

        throw error::reader_invalid_context{this}.message("must be in value context");
    }

    char _verify_eof(int value)
    {
        if (value == EOF) { throw error::reader_read_stream_error{this}.message("invalid end of file"); }
        return char(value);
    }

    void _step_context()
    {
        if (_scope.empty()) { return; }

        auto scope = &_scope.back();
        if (scope->elems_left-- == 0)
            throw error::reader_invalid_context{this}.message("all elements read");

        // TODO: if key, validate key context ...
    }

    scope_t const& _scope_ref() const
    {
        auto size = _scope.size();
        if (size == 0)
            throw error::reader_invalid_context{(if_reader*)this}.message("not in any valid scope!");

        return _scope[size - 1];
    }

    scope_t& _scope_ref() { return (scope_t&)((reader const*)this)->_scope_ref(); }

   private:
    static typecode _typecode(char v) noexcept
    {
        if (v & 0xe0) { return typecode::negative_fixint; }
        if (v & 0xc0) { return typecode(v); }
        if ((v & 0x7f) == 0) { return typecode::positive_fixint; }
        if (v & 0b101'00000) { return typecode::fixstr; }
        if (v & 0b1000'0000) { return typecode::fixmap; }
        if (v & 0b1001'0000) { return typecode::fixarray; }

        return typecode::error;
    }
};

}  // namespace CPPHEADERS_NS_::archive::msgpack
