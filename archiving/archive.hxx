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
#include <string_view>

#include "../template_utils.hxx"

//
#include "../__namespace__.h"

/**
 * Defines SAX-like interface for parsing / archiving
 */
namespace CPPHEADERS_NS_::archive {

class if_serializer
{
   public:
    virtual ~if_serializer() = default;

   public:
    std::function<void(char const*, size_t)> write;

   public:
    // clear internal context
    virtual void clear() = 0;

    virtual void push_object() = 0;
    virtual void push_array()  = 0;

    virtual void pop() {}

    virtual void add_key(std::string_view key) = 0;

    virtual void add_string(std::string_view key) { add_binary(key.data(), key.size()); };
    virtual void add_binary(void const* data, size_t n) = 0;

    virtual void add_null() = 0;

    virtual void add_bool(bool v) { add_int8(v); }

    virtual void add_int8(int8_t v) { add_int16(v); }
    virtual void add_int16(int16_t v) { add_int32(v); }
    virtual void add_int32(int32_t v) { add_int64(v); }
    virtual void add_int64(int64_t v) { add_double(v); }

    virtual void add_float(float v) { add_double(v); }
    virtual void add_double(double v) = 0;
};

class if_deserializer
{
   public:
    virtual ~if_deserializer() = default;

   public:
    std::function<void(char const*, size_t)> read;

   public:
};

template <typename Type_>
class metadata
{
   private:
    using serialize_function = void (*)(Type_ const&, if_serializer*);
    using serl_inst          = singleton<serialize_function, metadata<Type_>>;

   public:
    static auto serialize_fn() { return serl_inst::get(); }

    template <typename DeserlHandler_>
    static nullptr_t setup(serialize_function serializer)
    {
        serl_inst::get() = serializer;
        return nullptr;
    }
};

}  // namespace CPPHEADERS_NS_::archive
