
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
#include <utility>
#include <vector>

#include "../utility/singleton.hxx"
#include "if_archive.hxx"

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_::refl {
using archive::binary_t;

/**
 * List of available property formats
 */
enum class primitive_t : uint16_t
{
    invalid,

    map,
    array,

    null,
    boolean,
    integer,
    floating_point,
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

// alias
using object_descriptor_fn = std::function<object_descriptor*()>;

/**
 * Object's sub property info
 */
struct property_info
{
    //! Offset from object root
    size_t offset = 0;

    //! Object descriptor for this property
    object_descriptor_fn descriptor;

    //! Set to default function
    std::function<void(void*)> set_to_default_fn;

   public:
    property_info() = default;
    property_info(
            size_t offset,
            object_descriptor_fn descriptor,
            std::function<void(void*)> set_to_default_fn)
            : offset(offset),
              descriptor(std::move(descriptor)),
              set_to_default_fn(std::move(set_to_default_fn)) {}
};

/**
 * Required basic manipulator
 */
class if_primitive_manipulator
{
    virtual primitive_t type() const noexcept = 0;

    virtual size_t extent() const noexcept = 0;

    virtual void archive(archive::if_writer*, void const* pvdata) = 0;

    virtual void restore(archive::if_reader*, void* pvdata) = 0;

    // TODO: logics for runtime reflection/dynamic object manipulation ...
    //       - ctor/dtor
    //       - value copy function
    //       - value setter/getter
};

/**
 * SFINAE helper for overloading get_object_descriptor
 */
template <bool Test_>
using object_sfinae_t = std::enable_if_t<Test_, object_descriptor const*>;

template <typename ValTy_>
auto get_object_descriptor() -> object_sfinae_t<std::is_same_v<void, ValTy_>>;

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

    // property manipulator
    // if it's user defined object, it'll be nullptr.
    std::function<if_primitive_manipulator*()> _manip = nullptr;

    // list of properties
    // properties are not incremental in memory address, as they are
    //  simply stored in creation order.
    std::vector<property_info> _props;

    // [optional]
    // list of keys. if this exists, this object indicate an object.
    // otherwise, array.
    std::map<std::string, size_t> _keys;

   private:
    /*  Transients  */

    // Check if this descriptor is initialized
    bool _initialized = false;

    // Sorted array for property address ~ index mapping
    std::vector<std::pair<size_t /*offset*/, size_t /*prop_index*/>> _offset_lookup;

   public:
    /**
     * Archive/Serialization will
     */
    bool is_user_object() const noexcept { return _manip == nullptr; }

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

   public:
    void _archive_to(archive::if_writer*, object_data_t const*) const {}
    void _restore_from(archive::if_reader*, object_data_t*) const {}
};

namespace descriptor {
class basic_factory
{
   public:
    virtual ~basic_factory() = default;

   protected:
    object_descriptor _current;

   protected:
    size_t add_property_impl(property_info info);

   public:
    /**
     * Generate object descriptor instance.
     * Sorts keys, build incremental offset lookup table, etc.
     */
    object_descriptor generate();
};

class primitive_factory : public basic_factory
{
   public:
    void setup(std::function<if_primitive_manipulator*()> func)
    {
    }
};

class map_factory : public basic_factory
{
   public:
    void add_property(std::string key, property_info info);
};

class tuple_factory : public basic_factory
{
   public:
    void add_property(property_info info);
};
}  // namespace descriptor

/**
 * Dump object to archive
 */
inline archive::if_writer&
operator<<(archive::if_writer& strm, object_const_view_t obj)
{
    obj.meta->_archive_to(&strm, obj.data);
    return strm;
}

/**
 * Restore object from archive
 */
inline archive::if_reader&
operator>>(archive::if_reader& strm, object_view_t obj)
{
    obj.meta->_restore_from(&strm, obj.data);
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
    meta = get_object_descriptor<Ty_>();
}

template <typename Ty_>
object_const_view_t::object_const_view_t(const Ty_* p) noexcept : data((object_data_t*)p)
{
    meta = get_object_descriptor<Ty_>();
}

/**
 * Autogenerates integral property descriptor
 */
template <typename ValTy_>
auto get_object_descriptor() -> object_sfinae_t<std::is_integral_v<ValTy_>>
{
    static struct manip_t : if_primitive_manipulator
    {
       private:
        primitive_t type() const noexcept override
        {
            return primitive_t::integer;
        }
        size_t extent() const noexcept override
        {
            return sizeof(ValTy_);
        }
        void archive(archive::if_writer* writer, const void* p_void) override
        {
            *writer << *(ValTy_*)p_void;
        }
        void restore(archive::if_reader* reader, void* p_void) override
        {
            *reader >> *(ValTy_*)p_void;
        }
    } manip;

    static object_descriptor desc = [] {
        descriptor::primitive_factory factory;
        factory.setup([] { return &manip; });
        return factory.generate();
    }();

    return &desc;
}

inline void fc()
{
    get_object_descriptor<int>();
}

}  // namespace CPPHEADERS_NS_::refl

namespace std {

inline std::string to_string(CPPHEADERS_NS_::refl::primitive_t t)
{
    switch (t)
    {
        case perfkit::refl::primitive_t::invalid: return "invalid";
        case perfkit::refl::primitive_t::null: return "null";
        case perfkit::refl::primitive_t::boolean: return "boolean";
        case perfkit::refl::primitive_t::string: return "string";
        case perfkit::refl::primitive_t::binary: return "binary";
        case perfkit::refl::primitive_t::map: return "map";
        case perfkit::refl::primitive_t::array: return "array";
        case perfkit::refl::primitive_t::integer: return "integer";
        case perfkit::refl::primitive_t::floating_point: return "floating_point";
        default: return "__NONE__";
    }
}

}  // namespace std