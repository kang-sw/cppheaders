
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

#include <functional>
#include <map>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "../../utility/singleton.hxx"
#include "../algorithm.hxx"
#include "../counter.hxx"
#include "../if_archive.hxx"

//
#include "../../__namespace__.h"

namespace CPPHEADERS_NS_::refl {
using binary_t = archive::binary_t;

namespace error {
CPPH_DECLARE_EXCEPTION(object_exception, basic_exception<object_exception>);

struct object_archive_exception : object_exception
{
    archive::error_info error;

    object_archive_exception&& set(archive::if_archive_base* archive)
    {
        error = archive->dump_error();
        return std::move(*this);
    }
};

CPPH_DECLARE_EXCEPTION(invalid_read_state, object_archive_exception);
CPPH_DECLARE_EXCEPTION(invalid_write_state, object_archive_exception);
CPPH_DECLARE_EXCEPTION(missing_entity, object_archive_exception);

}  // namespace error

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
using object_descriptor_fn = std::function<object_descriptor const*()>;

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

    //! unique id for this property.
    int64_t unique_id = 0;

    //! index of self
    int index_self = 0;

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
enum class requirement_status_tag
{
    required = 0,
    optional = 1,

    optional_empty     = 1,  // assume default optional == empty
    optional_has_value = 2,
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
     * Underlying element type if this primitive is sort of container
     * This descriptor may not be used for actual data manipulation, but only for
     *  descriptive operations like validation/documentation/schema-generation.
     *
     * (optional, map, set, pointer, etc ...)
     */
    virtual object_descriptor const* element_type() const noexcept { return nullptr; };

    /**
     * Archive to writer
     */
    virtual void archive(archive::if_writer* strm, void const* pvdata) const
    {
        *strm << nullptr;
    }

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
    virtual requirement_status_tag status(void const* pvdata = nullptr) const noexcept
    {
        // NOTE: IMPLEMENTATION REQUIREMENT:
        //       - if 'pvdata' is empty, return 'required' or 'optional' as absolute type.
        //       - if 'pvdata' is non-null, return 'required' or 'optional_*' as current status
        return requirement_status_tag::required;
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
    using hierarchy_append_fn = std::function<void(object_descriptor const*,
                                                   property_info const*)>;

   private:
    /*   Properties   */

    // extent of this object
    size_t _extent = 0;

    // property manipulator
    // if it's user defined object, it'll be nullptr.
    std::function<if_primitive_manipulator*()> _primitive;

    // list of properties
    // properties are not incremental in memory address, as they are
    //  simply stored in creation order.
    std::vector<property_info> _props;

    // [optional]
    // list of keys. if this exists, this object indicate an object.
    // otherwise, array.
    std::optional<std::map<std::string, size_t, std::less<>>> _keys;

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
    bool is_primitive() const noexcept { return !!_primitive; }
    bool is_object() const noexcept { return _keys.has_value(); }
    bool is_tuple() const noexcept { return not is_primitive() && not is_object(); }
    bool is_optional() const noexcept { return is_primitive() && _primitive()->status() != requirement_status_tag::required; }

    /**
     * Current requirement status of given property
     */
    requirement_status_tag requirement_status(object_data_t const* data = nullptr) const noexcept
    {
        if (not is_primitive())
            return requirement_status_tag::required;

        return _primitive()->status(data);
    }

    /**
     * Extent of this object
     */
    size_t extent() const noexcept
    {
        return _extent;
    }

    /**
     * Retrieves data pointer from an object
     */
    object_data_t* retrieve(object_data_t* data, property_info const& property) const
    {
        assert(_props.at(property.index_self).unique_id == property.unique_id);
        return (object_data_t*)((char*)data + property.offset);
    }

    /**
     * Retrieves data pointer from an object
     */
    object_data_t const* retrieve(object_data_t const* data, property_info const& property) const
    {
        assert(_props.at(property.index_self).unique_id == property.unique_id);
        return (object_data_t const*)((char const*)data + property.offset);
    }

    /**
     * Retrieves property info from child of this object
     *
     * @param parent parent data pointer
     * @param child child data pointer
     * @param append
     * @return Number of properties. ~size_t{} if error.
     */
    size_t property(
            object_data_t* parent,
            object_data_t* child,
            hierarchy_append_fn const& append) const
    {
        assert(parent <= child);
        auto ofst = (char*)child - (char*)parent;
        return _find_property(ofst, append);
    }

    /**
     * Find property by string key
     */
    property_info const* property(std::string_view key) const
    {
        if (not _keys) { return nullptr; }  // this is not object.

        auto ptr = perfkit::find_ptr(*_keys, key);
        if (not ptr) { return nullptr; }

        return &_props.at(ptr->second);
    }

    /**
     * Get list of properties
     */
    decltype(_props) const& properties() const noexcept { return _props; }

    /**
     * Check if this is initialized object descriptor
     */
    bool is_valid() const noexcept { return _initialized; }

    /**
     * Create default initialized dynamic object
     */
    dynamic_object_ptr create() const;

    /**
     * Clone dynamic object from template
     */
    dynamic_object_ptr clone(object_data_t* parent) const;

   public:
    void _archive_to(archive::if_writer* strm, object_data_t const* data) const
    {
        // 1. iterate properties, and access to their object descriptor
        // 2. recursively call _archive_to on them.
        // 3. if 'this' is object, first add key on archive before recurse.

        if (is_primitive())
        {
            _primitive()->archive(strm, data);
        }
        else
        {
            if (is_object())
            {
                strm->object_push();
                for (auto& [key, index] : *_keys)
                {
                    if (not strm->is_key_next())
                        throw error::invalid_write_state{}
                                .set(strm)
                                .message("'key' expected.");

                    auto& prop = _props.at(index);

                    auto child      = prop.descriptor();
                    auto child_data = retrieve(data, prop);
                    assert(child_data);

                    if (auto status = child->requirement_status(child_data);
                        status == requirement_status_tag::optional_empty)
                    {
                        continue;  // skip empty optional property
                    }
                    else
                    {
                        *strm << key;
                        child->_archive_to(strm, child_data);
                    }
                }
                strm->object_pop();
            }
            else if (is_tuple())
            {
                strm->array_push();
                for (auto& prop : _props)
                {
                    auto child      = prop.descriptor();
                    auto child_data = retrieve(data, prop);
                    assert(child_data);

                    if (auto status = child->requirement_status(child_data);
                        status == requirement_status_tag::optional_empty)
                    {
                        // archive empty argument as null.
                        *strm << nullptr;
                    }
                    else
                    {
                        child->_archive_to(strm, child_data);
                    }
                }
                strm->array_pop();
            }
            else
            {
                assert(("Invalid code path", false));
            }
        }
    }

    struct restore_context
    {
        std::string keybuf;
        std::vector<bool> found_elems;
    };

    void _restore_from(
            archive::if_reader* strm,
            object_data_t* data,
            restore_context* context  // for reusing key buffer during recursive
    ) const
    {
        // 1. iterate properties, and access to their object descriptor
        // 2. recursively call _restore_from on them.
        // 3. if 'this' is object, first retrieve key from archive before recurse.
        //    if target property is optional, ignore missing key

        if (is_primitive())
        {
            _primitive()->restore(strm, data);
        }
        else if (is_object())
        {
            if (not strm->is_object_next())
                throw error::invalid_read_state{}
                        .set(strm)
                        .message("'object' expected");

            int hlevel = 0, hid = 0;
            strm->hierarchy(&hlevel, &hid);

            auto& found = context->found_elems;
            found.resize(_props.size(), false);

            while (not strm->should_break(hlevel, hid))
            {
                if (not strm->is_key_next())
                    throw error::invalid_read_state{}
                            .set(strm)
                            .message("'key' expected");

                // retrive key, and find it from my properties list
                auto& keybuf = context->keybuf;
                *strm >> keybuf;
                auto elem = find_ptr(*_keys, keybuf);

                // simply ignore unexpected keys
                if (not elem) { continue; }

                auto index   = elem->second;
                found[index] = true;

                auto& prop      = _props.at(index);
                auto child      = prop.descriptor();
                auto child_data = child->retrieve(data, prop);
                assert(child_data);

                child->_restore_from(strm, child_data, context);
            }

            for (auto index : perfkit::count(found.size()))
            {
                if (found[index]) { continue; }

                auto& prop = _props[index];
                auto child = prop.descriptor();

                if (not child->is_optional())
                    throw error::missing_entity{}
                            .message("property [%d: in offset %d] missing",
                                     (int)index, (int)prop.offset);
            }
        }
        else if (is_tuple())
        {
            if (not strm->is_array_next())
                throw error::invalid_read_state{}
                        .set(strm)
                        .message("'array' expected");

            int hlevel = 0, hid = 0;
            strm->hierarchy(&hlevel, &hid);

            // tuple is always in fixed-order
            for (auto& prop : _props)
            {
                if (strm->should_break(hlevel, hid))
                    throw error::missing_entity{}
                            .set(strm)
                            .message("Only %d out of %d properties found.",
                                     (int)(&prop - _props.data()), (int)_props.size());

                auto child = prop.descriptor();
                if (child->is_optional() && strm->is_null_next())
                {
                    // do nothing
                    nullptr_t nul;
                    *strm >> nul;
                    continue;
                }

                auto child_data = child->retrieve(data, prop);
                child->_restore_from(strm, child_data, context);
            }
        }
        else
        {
            assert(("Invalid code path", false));
        }
    }

   private:
    size_t _find_property(size_t offset, hierarchy_append_fn const& append, size_t depth = 0) const
    {
        // 1. find lower bound of given offet
        // 2. if offset value does not match exatly, check lower bound elem's type.
        // 3. if type is object or tuple, dig in.
        //    otherwise, it is incorrect access.
        auto at = [this](auto&& pair) { return &_props.at(pair.second); };

        auto it = lower_bound(_offset_lookup, std::make_pair(offset, ~size_t{}));
        if (it == _offset_lookup.end()) { return ~size_t{}; }

        // since second argument was given as ~size_t{}, it never hits exact value at once
        if (it != _offset_lookup.begin()) { it--; }

        // selected node's offset exceeds query -> invalid query
        if (it->first > offset) { return ~size_t{}; };

        // check for exact match
        if (it->first == offset) { return append(this, at(*it)), depth; }

        // otherwise, check for object/tuple
        auto& property = *at(*it);
        auto descr     = property.descriptor();

        // primitive cannot have children
        if (descr->is_primitive()) { return ~size_t{}; }

        // do recurse
        return descr->_find_property(offset - property.offset, append, ++depth);
    }

    property_info const* _find_property(size_t offset) const
    {
        // find hierarchy shallowly
        auto it = lower_bound(_offset_lookup, offset, [](auto& a, auto& b) { return a.first < b; });
        if (it == _offset_lookup.end()) { return nullptr; }
        if (it->first != offset) { return nullptr; }

        return &_props.at(it->second);
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
        size_t add_property_impl(property_info info)
        {
            assert(not _current->_primitive);

            auto index = _current->_props.size();
            _current->_props.emplace_back(std::move(info));

            return index;
        }

       public:
        /**
         * Generate object descriptor instance.
         * Sorts keys, build incremental offset lookup table, etc.
         */
        object_descriptor create()
        {
            static std::atomic_uint64_t unique_id_allocator;

            auto generated = *_current;
            auto lookup    = &generated._offset_lookup;

            lookup->reserve(generated._props.size());

            size_t n = 0;
            for (auto& prop : generated._props)
            {
                lookup->emplace_back(std::make_pair(prop.offset, n));
                prop.unique_id  = ++unique_id_allocator;
                prop.index_self = n++;
            }

            // simply sort incrementally.
            std::sort(lookup->begin(), lookup->end());

            assert("Offset duplication"
                   && adjacent_find(
                              *lookup, [](auto& a, auto& b) {
                                  return a.first == b.first;
                              })
                              != lookup->end());

            generated._initialized = true;  // set initialized
            return generated;
        }
    };

    class primitive_factory : public basic_factory
    {
       public:
        void setup(size_t extent, std::function<if_primitive_manipulator*()> func)
        {
            *_current = {};

            _current->_extent    = extent;
            _current->_primitive = std::move(func);
        }
    };

    class map_factory : public basic_factory
    {
       public:
        void start(size_t extent)
        {
            *_current         = {};
            _current->_extent = extent;
            _current->_keys.emplace();
        }

        void add_property(std::string key, property_info info)
        {
            auto index  = add_property_impl(std::move(info));
            auto is_new = _current->_keys->try_emplace(std::move(key), index).second;

            assert("Key must be unique" && is_new);
        }
    };

    class tuple_factory : public basic_factory
    {
       public:
        void start(size_t extent)
        {
            *_current         = {};
            _current->_extent = extent;
        }

        void add_property(property_info info)
        {
            add_property_impl(std::move(info));
        }
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
    object_descriptor::restore_context ctx;
    obj.meta->_restore_from(&strm, obj.data, &ctx);
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
        case CPPHEADERS_NS_::refl::primitive_t::invalid: return "invalid";
        case CPPHEADERS_NS_::refl::primitive_t::null: return "null";
        case CPPHEADERS_NS_::refl::primitive_t::boolean: return "boolean";
        case CPPHEADERS_NS_::refl::primitive_t::string: return "string";
        case CPPHEADERS_NS_::refl::primitive_t::binary: return "binary";
        case CPPHEADERS_NS_::refl::primitive_t::map: return "map";
        case CPPHEADERS_NS_::refl::primitive_t::array: return "array";
        case CPPHEADERS_NS_::refl::primitive_t::integer: return "integer";
        case CPPHEADERS_NS_::refl::primitive_t::floating_point: return "floating_point";
        default: return "__NONE__";
    }
}

}  // namespace std
