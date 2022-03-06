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
#include "detail/msgpack.hxx"

namespace CPPHEADERS_NS_::archive::msgpack {
class writer : public archive::if_writer
{
   private:
    detail::write_context_helper _ctx;

   public:
    void clear() override { if_writer::clear(), _ctx.clear(); }

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
            throw archive::error::writer_out_of_range{this, "size exceeds 32bit range"};
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
}  // namespace CPPHEADERS_NS_::archive::msgpack
