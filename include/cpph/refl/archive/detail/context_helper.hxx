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

#include "../../detail/if_archive.hxx"

namespace cpph::archive::error {
CPPH_DECLARE_EXCEPTION(writer_out_of_range, writer_invalid_state);
}

/**
 * Provides generic key-value based object archive context management
 */
namespace cpph::archive::detail {

class write_context_helper
{
    enum class scope_type : int8_t {
        invalid,
        object,
        array,
        binary
    };

    struct scoped_context_t {
        scope_type type = scope_type::invalid;
        bool key_ready = false;

        size_t size = 0;
        size_t capacity = 0;

        bool is_key_next() const
        {
            if (type == scope_type::object)
                return not(size & 1);
            else
                return false;
        }

        bool is_key_current() const
        {
            if (type == scope_type::object)
                return (size & 1);
            else
                return false;
        }
    };

   public:
    if_writer* self;

   private:
    std::vector<scoped_context_t> _scopes;

   public:
    //! Reserve depth of objects
    void reserve_depth(size_t n)
    {
        _scopes.reserve(n);
    }

    bool empty() const
    {
        return _scopes.empty();
    }

    size_t depth() const
    {
        return _scopes.size();
    }

    //! Clears all internal state
    void clear()
    {
        _scopes.clear();
    }

    //! Assert next entity is key
    //! @return true if comma required before write.
    bool write_key_next()
    {
        _assert_scope_not_empty();
        auto elem = &_scopes.back();

        if (not elem->is_key_next())
            throw error::writer_invalid_state{self, "Object key expected"};

        bool comma_required = elem->size++ > 0;
        elem->key_ready = true;

        _assert_scope_size_valid(elem);
        return comma_required;
    }

    //! Assert write key or value next.
    //! @return [bNeedComma, bIsKey, bNeedIndent]
    struct write_directive {
        bool is_key      : 1;
        bool need_comma  : 1;
        bool need_indent : 1;
    };

    write_directive write_next()
    {
        if (_scopes.empty()) { return {false, false, false}; }
        auto elem = &_scopes.back();

        if (elem->type != scope_type::object) {
            return {false, _write_value_next(), true};
        }

        if (elem->is_key_current()) {
            if (elem->key_ready) {
                // it's valid to write a key
                elem->key_ready = false;
                return {true, elem->size > 2, true};
            } else {
                _write_value_next();
                return {false, false, false};
            }
        } else {
            // after writing a value, context stay invalid until next call to write_next_key
            // invalid state can be known from key_ready field.
            throw error::writer_invalid_state{self, "write_key_next() is not called!"};
        }
    }

    //! Push array with n arguments
    void push_array(size_t n)
    {
        _assert_value_context();
        auto elem = &_scopes.emplace_back();
        elem->type = scope_type::array;
        elem->capacity = n;
    }

    //! Push object with n arguments
    void push_object(size_t n)
    {
        _assert_value_context();
        auto elem = &_scopes.emplace_back();
        elem->type = scope_type::object;
        elem->capacity = n * 2;
    }

    //! Push binary of size n
    void push_binary(size_t n)
    {
        _assert_value_context();
        auto elem = &_scopes.emplace_back();
        elem->type = scope_type::binary;
        elem->capacity = n;
    }

    //! Write n bytes of binary
    void binary_write_some(size_t n)
    {
        _assert_scope_not_empty();

        auto elem = &_scopes.back();
        elem->size += n;

        _assert_scope_size_valid(elem);
    }

    //! Pop array
    //! @return number of elements
    size_t pop_array()
    {
        _assert_top_scope_finished(scope_type::array);
        auto rv = _scopes.back().size;
        _scopes.pop_back();

        return rv;
    }

    //! Pop object
    size_t pop_object()
    {
        _assert_top_scope_finished(scope_type::object);
        auto rv = _scopes.back().size;
        _scopes.pop_back();

        return rv / 2;
    }

    //! Pop binary.
    size_t pop_binary()
    {
        _assert_top_scope_finished(scope_type::binary);
        auto rv = _scopes.back().size;
        _scopes.pop_back();

        return rv;
    }

   private:
    void _assert_scope_not_empty()
    {
        if (_scopes.empty())
            throw error::writer_invalid_state{self, "object must not be empty!"};
    }

    void _assert_value_context()
    {
        if (_scopes.empty()) { return; }

        if (_scopes.back().is_key_current())
            throw error::writer_invalid_state{self, "value expected!"};
    }

    void _assert_scope_size_valid(scoped_context_t* scope)
    {
        if (scope->size > scope->capacity)
            throw error::writer_out_of_range{
                    self, "invalid size %lld (max %lld)", scope->size, scope->capacity};
    }

    void _assert_top_scope_finished(scope_type t)
    {
        _assert_scope_not_empty();
        auto elem = &_scopes.back();
        _assert_scope_size_valid(elem);

        if (elem->size < elem->capacity)
            throw error::writer_invalid_state{
                    self, "only %lld out of %lld elements filled!", elem->size, elem->capacity};
    }

    bool _write_value_next()
    {
        if (_scopes.empty())
            return false;  // if it's empty scope, simply push value back.

        auto elem = &_scopes.back();
        bool comma_required = false;

        if (elem->type != scope_type::object)  // object value must not have comma.
            comma_required = elem->size > 0;

        ++elem->size;
        _assert_scope_size_valid(elem);

        return comma_required;
    }
};

class read_context_helper
{
   public:
};

}  // namespace cpph::archive::detail
