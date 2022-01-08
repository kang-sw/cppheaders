
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

#include "../utility/singleton.hxx"
#include "if_archive.hxx"

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_::refl {

/**
 * List of available property formats
 */
enum class format_t : uint16_t
{
    invalid,

    _objects_,
    object,
    object_pointer,
    tuple,

    _primitives_,
    null,
    boolean,
    i8,
    i16,
    i32,
    i64,
    f32,
    f64,
    string,
    binary,
};

/**
 * dummy incomplete class type to give objects type resolution
 */
class object_data_t;
class object_descriptor;  // forwarding

/**
 * Object/metadata wrapper for various use
 */
struct object_view_t
{
    object_descriptor const* meta = {};
    object_data_t* data           = {};

   public:
    object_view_t() = default;
    object_view_t(const object_descriptor* meta, object_data_t* data) : meta(meta), data(data) {}

    template <typename Ty_>
    explicit object_view_t(Ty_* p) noexcept;

   public:
    auto pair() const noexcept { return std::make_pair(meta, data); }
};

struct object_const_view_t
{
    object_descriptor const* meta = {};
    object_data_t const* data     = {};

   public:
    object_const_view_t() = default;
    object_const_view_t(const object_descriptor* meta, const object_data_t* data) : meta(meta), data(data) {}

    template <typename Ty_>
    explicit object_const_view_t(Ty_ const* p) noexcept;

   public:
    auto pair() const noexcept { return std::make_pair(meta, data); }
};

/**
 * A descriptor for runtime object field entity
 *
 * Manages object lifecycle, etc.
 *
 * TODO: dynamic object manipulation
 */
struct dynamic_object_ptr
{
   private:
    object_descriptor const* _meta = {};
    object_data_t* _data           = {};

   public:
    /*  Lifetime Management  */

   public:
    auto pair() const noexcept { return std::make_pair(_meta, _data); }
    operator object_view_t() noexcept { return {_meta, _data}; }
    operator object_const_view_t() noexcept { return {_meta, _data}; }
};

struct property_info
{
    format_t type = format_t::invalid;

    size_t offset = 0;  // offset from object root
    size_t extent = 0;  // size of this property

    void (*write_fn)(archive::if_writer*, object_const_view_t) = nullptr;
    void (*read_fn)(archive::if_reader*, object_view_t)        = nullptr;

    // [optional]
    // Gets descriptor from this property.
    // Only valid when the type is object.
    // Declared callable object, to support dynamically generated objects
    std::function<object_descriptor*()> descriptor;

    // TODO: default value setter

    // TODO: value copy function

    // TODO: value setter/getter
};

/**
 * Object descriptor, which can manipulate random object.
 *
 * @warning There is no way to perform dynamic type recognition only with data pointer!
 *          If you have any plan to manipulate objects without static type information,
 *           manipulate them with wrapped_object or object_pointer!
 */
class object_descriptor
{
   private:
    friend class object_descriptor_factory;
    /*   Properties   */

    // list of properties
    // properties are not incremental in memory address, as they are
    //  simply stored in creation order.
    std::vector<property_info> _props;

    // [optional]
    // list of keys. if this exists, this object indicate an object.
    // otherwise, array.
    std::optional<std::map<std::string, size_t>> _keys;

   private:
    /*  Transients  */

    // Check if this descriptor is initialized
    bool _initialized = false;

    // Sorted array for property address ~ index mapping
    std::vector<std::pair<size_t /*offset*/, size_t /*prop_index*/>> _offset_lookup;

   public:
    /**
     * Retrieves data pointer from an object
     */
    object_data_t* retrieve(object_data_t* data, property_info const& property) const;

    /**
     * Retrieves property info from child of this object
     */
    property_info const* property(object_data_t* parent, object_data_t* child);

    /**
     * Find property by string key
     */
    property_info const* property(std::string_view key) const;

    /**
     * Get list of properties
     */
    decltype(_props) const& properties() const;

    /**
     * Check if this is initialized object descriptor
     */
    bool is_valid() const noexcept { return _initialized; }

    /**
     * Create default initialized dynamic object
     */
    dynamic_object_ptr create();

    /**
     * Clone dynamic object from template
     */
    dynamic_object_ptr clone(object_data_t* parent);
};

class object_descriptor_factory
{
   public:
    /**
     * Create factory for empty object descriptor
     */
    static object_descriptor_factory create();

    /**
     * Create factory for extending existing object descriptor
     */
    static object_descriptor_factory based_on(object_descriptor const&);

   public:
    // add property

    // remove property
};

/**
 * Dump object to archive
 */
inline archive::if_writer&
operator<<(archive::if_writer& strm, object_const_view_t obj)
{
    return strm;
}

/**
 * Restore object from archive
 */
inline archive::if_reader&
operator>>(archive::if_reader& strm, object_view_t obj)
{
    return strm;
}

}  // namespace CPPHEADERS_NS_::refl

/**
 * Implmentations
 */
namespace CPPHEADERS_NS_::refl {

template <typename Ty_>
object_view_t::object_view_t(Ty_* p) noexcept : data((object_data_t*)p)
{
    if (singleton<object_descriptor, Ty_>->is_valid())
        meta = &*singleton<object_descriptor, Ty_>;
}

template <typename Ty_>
object_const_view_t::object_const_view_t(const Ty_* p) noexcept : data((object_data_t*)p)
{
    if (singleton<object_descriptor, Ty_>->is_valid())
        meta = &*singleton<object_descriptor, Ty_>;
}

}  // namespace CPPHEADERS_NS_::refl

namespace std {

inline std::string to_string(CPPHEADERS_NS_::refl::format_t t)
{
    switch (t)
    {
        case perfkit::refl::format_t::invalid: return "invalid";
        case perfkit::refl::format_t::_objects_: return "_objects_";
        case perfkit::refl::format_t::object: return "object";
        case perfkit::refl::format_t::object_pointer: return "object_pointer";
        case perfkit::refl::format_t::tuple: return "tuple";
        case perfkit::refl::format_t::_primitives_: return "_primitives_";
        case perfkit::refl::format_t::null: return "null";
        case perfkit::refl::format_t::boolean: return "boolean";
        case perfkit::refl::format_t::i8: return "i8";
        case perfkit::refl::format_t::i16: return "i16";
        case perfkit::refl::format_t::i32: return "i32";
        case perfkit::refl::format_t::i64: return "i64";
        case perfkit::refl::format_t::f32: return "f32";
        case perfkit::refl::format_t::f64: return "f64";
        case perfkit::refl::format_t::string: return "string";
        case perfkit::refl::format_t::binary: return "binary";
        default: return "__NONE__";
    }
}

}  // namespace std
