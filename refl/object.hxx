
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
#include <map>
#include <optional>
#include <string_view>
#include <vector>

#include "../template_utils.hxx"
#include "if_archive.hxx"

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_::reflect {

/**
 * List of available property formats
 */
enum class format_t : uint16_t
{
    invalid,

    null = 0x0010,

    object         = 0x020,
    object_pointer = 0x021,
    array          = 0x030,
    boolean        = 0x050,

    integer = 0x0100,
    i8,
    i16,
    i32,
    i64,

    floating_point = 0x0200,
    f32,
    f64,

    string = 0x0300,
    binary = 0x0400,
};

/**
 * dummy incomplete class type to give objects type resolution
 */
class object_data_t;
class basic_object_info;  // forwarding

/**
 * A descriptor for runtime object
 *
 * Manages object lifecycle, etc.
 */
struct object_pointer_t
{
   private:
    basic_object_info const* _meta = {};
    object_data_t* _data           = {};
};

struct property_info
{
    format_t type = format_t::invalid;

    size_t offset = 0;  // offset from object root
    size_t extent = 0;  // size of this property

    void (*write_fn)(archive::if_writer*, object_data_t*) = nullptr;
    void (*read_fn)(archive::if_reader*, object_data_t*)  = nullptr;
};

class basic_object_info
{
   public:
    object_data_t* retrieve(object_data_t*);

   private:
    friend class object_factory;
    std::vector<property_info> _props;
};

class object_factory
{
};

struct wrapped_object_t
{
    basic_object_info const* meta = {};
    object_data_t* data           = {};
};

/**
 * Dump object to archive
 */
inline archive::if_writer&
operator<<(archive::if_writer& strm, wrapped_object_t obj)
{
    return strm;
}

/**
 * Restore object from archive
 */


}  // namespace CPPHEADERS_NS_::reflect