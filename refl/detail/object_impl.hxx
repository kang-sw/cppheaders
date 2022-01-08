
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

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "../../utility/singleton.hxx"
#include "../if_archive.hxx"

//
#include "../../__namespace__.h"

namespace CPPHEADERS_NS_::refl {
using binary_t = archive::binary_t;

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
              set_to_default_fn(std::move(set_to_default_fn))
    {
    }
};

/**
 *
 */
enum class primitive_status
{
    required,
    optional_has_value,
    optional_empty,
};

/**
 * Required basic manipulator
 */
class if_primitive_manipulator
{
   public:
    /**
     * Return actual type of this primitive
     */
    virtual primitive_t type() const noexcept = 0;

    /**
     * Archive to writer
     */
    virtual void archive(archive::if_writer*, void const* pvdata) const = 0;

    /**
     * Restore from reader
     */
    virtual void restore(archive::if_reader*, void* pvdata) const = 0;

    /**
     * Check status of given parameter. Default is 'required', which cannot be ignored.
     * Otherwise return value indicates this primitive is optional, which can be skipped on
     *  archiving, and can be ignored if there's no corresponding key during restoration.
     *
     * On tuple, empty optional object will be serialized as null.
     */
    virtual primitive_status status(void const* pvdata) const noexcept
    {
        return primitive_status::required;
    }

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
    /*   Properties   */

    // extent of this object
    size_t _extent = 0;

    // property manipulator
    // if it's user defined object, it'll be nullptr.
    std::function<if_primitive_manipulator*()> _primitive = nullptr;

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
     * Archive/Serialization will
     */
    bool is_user_object() const noexcept { return _primitive == nullptr; }

    /**
     * Extent of this object
     */
    size_t extent() const noexcept { return _extent; }

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
    void _archive_to(archive::if_writer*, object_data_t const*) const
    {
        // 1. iterate properties, and access to their object descriptor
        // 2. recursively call _archive_to on them.
        // 3. if 'this' is object, first add key on archive before recurse.

        // TODO
    }

    void _restore_from(archive::if_reader*, object_data_t*) const
    {
        // 1. iterate properties, and access to their object descriptor
        // 2. recursively call _restore_from on them.
        // 3. if 'this' is object, first retrieve key from archive before recurse.
        //    if target property is optional, ignore missing key

        // TODO
    }

   private:
    property_info const* _find_property(size_t offset, size_t extent) const
    {
        // TODO
        // 1. find lower bound of given offet
        // 2. if it does not match
    }

   public:
    // Factory classes

    class basic_factory
    {
       public:
        virtual ~basic_factory() = default;

       protected:
        std::unique_ptr<object_descriptor> _current
                = std::make_unique<object_descriptor>();

       protected:
        size_t add_property_impl(property_info info);

       public:
        /**
         * Generate object descriptor instance.
         * Sorts keys, build incremental offset lookup table, etc.
         */
        object_descriptor generate()
        {
            // TODO: - set _intialized 'true'
            //       - build offset lookup table
            auto generated = *_current;
            auto lookup    = &generated._offset_lookup;

            lookup->reserve(generated._props.size());

            size_t n = 0;
            for (auto& prop : generated._props)
                lookup->emplace_back(std::make_pair(prop.offset, n++));

            // simply sort incrementally.
            std::sort(lookup->begin(), lookup->end());

            // check for offset duplication
            auto it_dup = std::adjacent_find(
                    lookup->begin(), lookup->end(),
                    [](auto& a, auto& b) { return a.first == b.first; });
            if (it_dup != lookup->end()) { throw std::logic_error{"offset duplication!"}; }

            return generated;
        }
    };

    class primitive_factory : public basic_factory
    {
       public:
        void setup(size_t extent, std::function<if_primitive_manipulator*()> func)
        {
            _current->_extent    = extent;
            _current->_primitive = func;
        }
    };

    class map_factory : public basic_factory
    {
       public:
        void start(size_t extent);
        void add_property(std::string key, property_info info);
    };

    class tuple_factory : public basic_factory
    {
       public:
        void start(size_t extent);
        void add_property(property_info info);
    };
};

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
