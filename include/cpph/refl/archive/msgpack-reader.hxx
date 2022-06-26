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

#include "../detail/if_archive.hxx"
#include "detail/msgpack.hxx"

namespace cpph::archive::msgpack {
CPPH_DECLARE_EXCEPTION(type_mismatch_exception, error::reader_recoverable_exception);

class reader : public archive::if_reader
{
    union key_t {
        context_key data;
        struct
        {
            uint32_t id;
            uint32_t index;
        };
    };

    struct scope_t {
        enum type_t {
            type_object,
            type_array,
            type_binary
        };

        key_t ctxkey;
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
        reserve_depth(reserved_depth);
    }

    void reserve_depth(size_t n) { _scope.reserve(n); }

    //! Clears internal parsing state
    void clear() override { if_reader::clear(), _scope.clear(), _scope_key_gen = 0; }

   private:
    template <typename ValTy_, typename CastTo_ = ValTy_>
    CastTo_ _get_n_bigE()
    {
        char buffer[sizeof(ValTy_)];
        for (int i = sizeof buffer - 1; i >= 0; --i)
            buffer[i] = _buf->sbumpc();

        return static_cast<CastTo_>(*reinterpret_cast<ValTy_*>(buffer));
    }

    template <typename ValTy_, typename CastTo_ = ValTy_>
    CastTo_ _bump_n_bigE()
    {
        _buf->sbumpc();
        return _get_n_bigE<ValTy_, CastTo_>();
    }

    constexpr static typecode _do_offset(typecode value, int n)
    {
        return typecode((int)value + n);
    }

    // Used for str8, bin8
    template <typecode Base_, int Ofst_ = 0, typecode Fix_ = typecode::error, size_t FixMask_ = 0,
              typename HeaderChar_,
              typename = std::enable_if_t<sizeof(HeaderChar_) == 1>>
    uint32_t _read_elem_count(HeaderChar_ header)
    {
        switch (_typecode(header)) {
            case Fix_: return _buf->sbumpc(), (uint8_t)header & FixMask_;
            case _do_offset(Base_, Ofst_): return _bump_n_bigE<uint8_t>();
            case _do_offset(Base_, Ofst_ + 1): return _bump_n_bigE<uint16_t>();
            case _do_offset(Base_, Ofst_ + 2): return _bump_n_bigE<uint32_t>();

            default:
                throw type_mismatch_exception{this, "type error"};
        }
    }

    uint32_t _read_elem_count_str(char header) { return _read_elem_count<typecode::str8, 0, typecode::fixstr, 31>(header); }
    uint32_t _read_elem_count_bin(char header) { return _read_elem_count<typecode::bin8, 0>(header); }
    uint32_t _read_elem_count_map(char header) { return _read_elem_count<typecode::map16, -1, typecode::fixmap, 15>(header); }
    uint32_t _read_elem_count_array(char header) { return _read_elem_count<typecode::array16, -1, typecode::fixarray, 15>(header); }

    // Used for array16, bin16
    double _parse_number(char header)
    {
        char buf[64];
        char buflen = _read_elem_count_str(header);

        if (buflen >= sizeof buf) {
            _discard_n_bytes(buflen);
            throw error::reader_recoverable_exception{this, "too big number"};
        }

        if (_buf->sgetn(buf, buflen) != buflen)
            throw error::reader_unexpected_end_of_file{this};

        buf[buflen] = '\0';
        char* tail;
        auto value = strtod(buf, &tail);

        if (tail - buf != buflen) {
            throw error::reader_recoverable_parse_failure{this, "given string is not a number"};
        }

        return value;
    }

    template <typename ValTy_>
    auto _read_number(char header)
    {
        switch (_typecode(header)) {
            case typecode::positive_fixint:
            case typecode::negative_fixint:
                _buf->sbumpc();
                return ValTy_(header);

            case typecode::bool_false: return _buf->sbumpc(), ValTy_(0);
            case typecode::bool_true: return _buf->sbumpc(), ValTy_(1);

            case typecode::float32: return _bump_n_bigE<float, ValTy_>();
            case typecode::float64: return _bump_n_bigE<double, ValTy_>();

            case typecode::uint8: return _bump_n_bigE<uint8_t, ValTy_>();
            case typecode::uint16: return _bump_n_bigE<uint16_t, ValTy_>();
            case typecode::uint32: return _bump_n_bigE<uint32_t, ValTy_>();
            case typecode::uint64: return _bump_n_bigE<uint64_t, ValTy_>();
            case typecode::int8: return _bump_n_bigE<int8_t, ValTy_>();
            case typecode::int16: return _bump_n_bigE<int16_t, ValTy_>();
            case typecode::int32: return _bump_n_bigE<int32_t, ValTy_>();
            case typecode::int64: return _bump_n_bigE<int64_t, ValTy_>();

            case typecode::fixstr:
            case typecode::str8:
            case typecode::str16:
            case typecode::str32:
                // return ValTy_(_parse_number(header));

            default:
                throw type_mismatch_exception{
                        (if_reader*)this, "number type expected: %02x", header};
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
        v = _read_number<bool>(_verify_eof(_buf->sgetc()));
        _step_context();
        return *this;
    }

   private:
    template <typename T_>
    if_reader& _quick_get_num(T_& ref)
    {
        ref = _read_number<T_>(_verify_eof(_buf->sgetc()));
        _step_context();
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
        auto header = _verify_eof(_buf->sgetc());
        uint32_t buflen = _read_elem_count_str(header);

        _step_context();

        v.resize(buflen);
        _buf->sgetn(v.data(), v.size());

        return *this;
    }

    size_t elem_left() const override { return _scope_ref().elems_left; }

    bool should_break(const context_key& key) const override
    {
        auto scope = &_scope.back();
        return key.value == scope->ctxkey.data.value && scope->elems_left == 0;
    }

    context_key begin_object() override
    {
        _verify_not_key_type();

        auto header = _verify_eof(_buf->sgetc());
        auto n_elem = _read_elem_count_map(header);

        _step_context();
        return _new_scope(scope_t::type_object, n_elem)->ctxkey.data;
    }

    void end_object(context_key key) override
    {
        auto nbrk = _calc_num_break_scope(scope_t::type_object, key);
        while (nbrk--) { _break_scope(); }
    }

    size_t begin_binary() override
    {
        _verify_not_key_type();

        auto header = _verify_eof(_buf->sgetc());
        uint32_t buflen = _read_elem_count_bin(header);

        _step_context();
        _new_scope(scope_t::type_binary, buflen);
        return buflen;
    }

    size_t binary_read_some(mutable_buffer_view v) override
    {
        auto scope = _verify_scope(scope_t::type_binary);
        auto n_read = std::min<uint32_t>(v.size(), scope->elems_left);

        if (_buf->sgetn(v.data(), n_read) != n_read)
            throw error::reader_unexpected_end_of_file{this};

        scope->elems_left -= n_read;
        return n_read;
    }

    void end_binary() override
    {
        // consume rest of bytes
        auto scope = _verify_scope(scope_t::type_binary);
        _discard_n_bytes(scope->elems_left);

        _scope.pop_back();
    }

    context_key begin_array() override
    {
        _verify_not_key_type();

        auto header = _verify_eof(_buf->sgetc());
        uint32_t n_elem = _read_elem_count_array(header);

        _step_context();
        return _new_scope(scope_t::type_array, n_elem)->ctxkey.data;
    }

    void end_array(context_key key) override
    {
        auto nbrk = _calc_num_break_scope(scope_t::type_array, key);
        while (nbrk--) { _break_scope(); }
    }

    void read_key_next() override
    {
        auto scope = _verify_scope(scope_t::type_object);

        if (scope->elems_left & 1)
            throw error::reader_invalid_context{this, "not a valid order for key!"};
        if (scope->reading_key)
            throw error::reader_invalid_context{this, "duplicated call for read_key_next()"};

        scope->reading_key = true;
    }

    entity_type type_next() const override
    {
        auto header = _verify_eof(_buf->sgetc());
        switch (_typecode(header)) {
            case typecode::float32:
            case typecode::float64:
                return entity_type::floating_point;

            case typecode::positive_fixint:
            case typecode::negative_fixint:
            case typecode::uint8:
            case typecode::uint16:
            case typecode::uint32:
            case typecode::uint64:
            case typecode::int8:
            case typecode::int16:
            case typecode::int32:
            case typecode::int64:
                return entity_type::integer;

            case typecode::bool_false:
            case typecode::bool_true:
                return entity_type::boolean;

            case typecode::fixstr:
            case typecode::str8:
            case typecode::str16:
            case typecode::str32:
                return entity_type::string;

            case typecode::bin8:
            case typecode::bin16:
            case typecode::bin32:
                return entity_type::binary;

            case typecode::fixarray:
            case typecode::array16:
            case typecode::array32:
                return entity_type::array;

            case typecode::fixmap:
            case typecode::map16:
            case typecode::map32:
                return entity_type::dictionary;

            case typecode::nil:
                return entity_type::null;

            default:
                throw error::reader_parse_failed{this, "unsupported format: %02x", header};
        }
    }

   private:
    void _break_scope()
    {
        for (auto scope = &_scope_ref(); scope->elems_left > 0;) {
            if (scope->type == scope_t::type_object && (scope->elems_left & 1) == 0)
                scope->reading_key = true;

            _skip_once();
        }

        _scope.pop_back();
    }

    uint32_t _skip_once()
    {
        // intentionally uses getc instead of bumpc
        auto header = _verify_eof(_buf->sgetc());
        bool require_step_context = true;
        uint32_t skip_bytes = 0;
        switch (_typecode(header)) {
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
                _read_number<uint64_t>(header);
                break;

            case typecode::fixstr:
            case typecode::str8:
            case typecode::str16:
            case typecode::str32:
                skip_bytes = _read_elem_count<typecode::str8, 0, typecode::fixstr, 31>(header);
                break;

            case typecode::bin8:
            case typecode::bin16:
            case typecode::bin32:
                skip_bytes = _read_elem_count<typecode::bin8>(header);
                break;

            case typecode::fixarray:
            case typecode::array16:
            case typecode::array32:
                require_step_context = false;
                end_array(begin_array());
                break;

            case typecode::fixmap:
            case typecode::map16:
            case typecode::map32:
                require_step_context = false;
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
                skip_bytes = _read_elem_count<typecode::ext8>(header) + 1;  // 1 for ext
                break;

            default:
            case typecode::error:
                throw error::reader_parse_failed{this, "unsupported format: %02x", header};
        }

        if (require_step_context)
            _step_context_on_skip();

        _discard_n_bytes(skip_bytes);
        return skip_bytes;
    }

    size_t _calc_num_break_scope(scope_t::type_t type, context_key key)
    {
        // check if given context key exists in scopes
        for (auto it = _scope.rbegin(), end = _scope.rend(); it != end; ++it)
            if (it->ctxkey.data.value == key.value) {
                if (it->type == type)
                    return it - _scope.rbegin() + 1;
                else
                    throw error::reader_check_failed{this, "type mismatch with context!"};
            }

        throw error::reader_invalid_context{this, "too early scope end call!"};
    }

    scope_t* _new_scope(scope_t::type_t ty, uint32_t n_elems)
    {
        auto scope = &_scope.emplace_back();
        scope->type = ty;
        scope->elems_left = n_elems * (ty == scope_t::type_object ? 2 : 1);
        scope->reading_key = false;
        scope->ctxkey.index = uint32_t(_scope.size() - 1);
        scope->ctxkey.id = ++_scope_key_gen;

        return scope;
    }

    void _verify_not_key_type()
    {
        if (_scope.empty()) { return; }

        auto const& scope = _scope.back();
        if (scope.type != scope_t::type_object) { return; }

        if ((scope.elems_left & 1) == 0)
            throw error::reader_check_failed{this, "context is in key order"};
        if (scope.reading_key)
            throw error::reader_check_failed{this, "reading_key is set"};
    }

    char _verify_eof(std::streambuf::traits_type::int_type value) const
    {
        if (value == std::streambuf::traits_type::eof())
            throw error::reader_unexpected_end_of_file{this};

        return char(value);
    }

    void _step_context()
    {
        if (_scope.empty()) { return; }

        auto scope = &_scope.back();
        if (scope->type == scope_t::type_binary)
            throw error::reader_check_failed{this, "binary can not have any subobject!"};

        if (scope->type == scope_t::type_object && not(scope->elems_left & 1)) {
            if (not scope->reading_key)
                throw error::reader_check_failed{this, "read_key_next is not called!"};
            else
                scope->reading_key = false;
        }

        if (scope->elems_left-- == 0)
            throw error::reader_check_failed{this, "all elements read"};
    }

    void _step_context_on_skip()
    {
        if (_scope.empty()) { return; }

        // On skipping context, do not validate object key-array context
        if (_scope.back().elems_left-- == 0)
            throw error::reader_check_failed{this, "all elements read"};
    }

    scope_t const& _scope_ref() const
    {
        auto size = _scope.size();
        if (size == 0)
            throw error::reader_check_failed{(if_reader*)this, "not in any valid scope!"};

        return _scope[size - 1];
    }

    scope_t& _scope_ref() { return (scope_t&)((reader const*)this)->_scope_ref(); }

    scope_t* _verify_scope(scope_t::type_t t)
    {
        auto scope = &_scope_ref();
        if (scope->type != t)
            throw error::reader_check_failed{
                    this, "invalid scope type: was %d - %d expected", scope->type, t};

        return scope;
    }

    void _discard_n_bytes(size_t bytes)
    {
        char buf[256];
        for (size_t to_read; bytes > 0; bytes -= to_read) {
            to_read = std::min(sizeof buf, bytes);

            if (_buf->sgetn(buf, to_read) != to_read)
                throw error::reader_unexpected_end_of_file{this};
        }
    }

   private:
    typecode _typecode(char v) const
    {
        switch ((v & 0xf0) >> 4) {
            case 0b1100:
            case 0b1101: return typecode(v);

            case 0b1001: return typecode::fixarray;
            case 0b1000: return typecode::fixmap;

            case 0b1110:
            case 0b1111: return typecode::negative_fixint;

            case 0b1010:
            case 0b1011: return typecode::fixstr;

            case 0b0000:
            case 0b0001:
            case 0b0010:
            case 0b0011:
            case 0b0100:
            case 0b0101:
            case 0b0110:
            case 0b0111: return typecode::positive_fixint;

            default:
                throw error::reader_parse_failed{this, "invalid typecode %d", v};
        }
    }
};

}  // namespace cpph::archive::msgpack
