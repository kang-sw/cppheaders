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

#include "context_helper.hxx"

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

    if_writer& operator<<(nullptr_t a_nullptr) override
    {
        _ctx.write_next();
        _ap(typecode::nil);
        return *this;
    }

    if_writer& operator<<(bool v) override
    {
        _ctx.write_next();
        _ap(v ? typecode::bool_true : typecode::bool_false);
        return *this;
    }

    if_writer& operator<<(std::string_view v) override
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

    if_writer& operator<<(float v) override
    {
        _ctx.write_next();
        _ap(typecode::float32), _putbin<float>(v);
        return *this;
    }

    if_writer& operator<<(double v) override
    {
        _ctx.write_next();
        _ap(typecode::float64), _putbin<double>(v);
        return *this;
    }

    if_writer& operator<<(int8_t v) override { return _write_int(v); }
    if_writer& operator<<(int16_t v) override { return _write_int(v); }
    if_writer& operator<<(int32_t v) override { return _write_int(v); }
    if_writer& operator<<(int64_t v) override { return _write_int(v); }

    if_writer& operator<<(uint8_t v) override { return _write_uint(v); }
    if_writer& operator<<(uint16_t v) override { return _write_uint(v); }
    if_writer& operator<<(uint32_t v) override { return _write_uint(v); }
    if_writer& operator<<(uint64_t v) override { return _write_uint(v); }

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
        context_key key;
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

        bool _reading_key = false;
    };

   private:
    std::vector<scope_t> _scope;

   public:
    explicit reader(std::streambuf* buf, size_t reserved_depth = 0)
            : archive::if_reader(buf)
    {
        _scope.reserve(reserved_depth);
    }

    void reserve_depth(size_t n) { _scope.reserve(n); }

   private:
    template <typecode... Args_>
    void _verify_type(typecode code) const
    {
        if (((code != Args_) || ...))
        {
            throw error::reader_parse_failed{(if_reader*)this}.message("[msgpack] type error");
        }
    }

    template <typecode... Args_>
    void _verify_type(char c) const
    {
        _verify_type<Args_...>(_typecode(c));
    }

    template <typename ValTy_, typename CastTo_ = ValTy_>
    CastTo_ _read_number() const
    {
        char buffer[sizeof(ValTy_)];
        for (int i = sizeof buffer - 1; i >= 0; --i)
            buffer[i] = _buf->sgetc();

        return static_cast<CastTo_>(*reinterpret_cast<ValTy_*>(buffer));
    }

    template <typename ValTy_>
    auto _get_number(char ch) const
    {
        auto const code = typecode(ch);
        switch (code)
        {
            case typecode::positive_fixint:
            case typecode::negative_fixint:
                return ValTy_(ch);

            case typecode::bool_false: return ValTy_(0);
            case typecode::bool_true: return ValTy_(1);

            case typecode::float32: return _read_number<float, ValTy_>();
            case typecode::float64: return _read_number<double, ValTy_>();

            case typecode::uint8: return _read_number<uint8_t, ValTy_>();
            case typecode::uint16: return _read_number<uint16_t, ValTy_>();
            case typecode::uint32: return _read_number<uint32_t, ValTy_>();
            case typecode::uint64: return _read_number<uint64_t, ValTy_>();
            case typecode::int8: return _read_number<int8_t, ValTy_>();
            case typecode::int16: return _read_number<int16_t, ValTy_>();
            case typecode::int32: return _read_number<int32_t, ValTy_>();
            case typecode::int64: return _read_number<int64_t, ValTy_>();

            default:
                throw error::reader_parse_failed{(if_reader*)this}.message("[msgpack] number type expected: %d", code);
        }
    }

   public:
    if_reader& operator>>(nullptr_t a_nullptr) override
    {
        _step_context();
        _verify_type<typecode::nil>(_buf->sgetc());

        return *this;
    }

    if_reader& operator>>(bool& v) override
    {
        _step_context();
        v = _get_number<bool>(_buf->sgetc());
        return *this;
    }

   private:
    template <typename T_>
    if_reader& _quick_get_num(T_& ref)
    {
        _step_context();
        ref = _get_number<T_>(_buf->sgetc());
        return *this;
    }

   public:
    if_reader& operator>>(int8_t& v) override { return _quick_get_num(v); }
    if_reader& operator>>(int16_t& v) override { return _quick_get_num(v); }
    if_reader& operator>>(int32_t& v) override { return _quick_get_num(v); }
    if_reader& operator>>(int64_t& v) override { return _quick_get_num(v); }
    if_reader& operator>>(uint8_t& v) override { return _quick_get_num(v); }
    if_reader& operator>>(uint16_t& v) override { return _quick_get_num(v); }
    if_reader& operator>>(uint32_t& v) override { return _quick_get_num(v); }
    if_reader& operator>>(uint64_t& v) override { return _quick_get_num(v); }
    if_reader& operator>>(float& v) override { return _quick_get_num(v); }
    if_reader& operator>>(double& v) override { return _quick_get_num(v); }

    if_reader& operator>>(std::string& v) override;

    size_t elem_left() const override;
    size_t begin_binary() override;

    if_reader& binary_read_some(mutable_buffer_view v) override;

    void end_binary() override;

    context_key begin_object() override;
    context_key begin_array() override;

    bool should_break(const context_key& key) const override;

    void end_object(context_key key) override;
    void end_array(context_key key) override;
    void read_key_next() override;

    bool is_null_next() const override;
    bool is_object_next() const override;
    bool is_array_next() const override;

   private:
    void _step_context()
    {
        if (_scope.empty()) { return; }

        auto scope = &_scope.back();
        if (scope->elems_left == 0) { throw error::reader_invalid_context{this}.message("[msgpack] scope already finished"); }
        --scope->elems_left;
    }

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
