
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

#include <algorithm>
#include <atomic>
#include <map>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "../../__namespace__"
#include "../../algorithm/std.hxx"
#include "../../container/sorted_vector.hxx"
#include "../../counter.hxx"
#include "if_archive.hxx"

namespace CPPHEADERS_NS_::refl {
class object_metadata;

using object_metadata_t          = object_metadata const*;
using object_metadata_ptr        = std::unique_ptr<object_metadata const>;
using optional_property_metadata = struct property_metadata const*;

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

CPPH_DECLARE_EXCEPTION(primitive, object_archive_exception);
CPPH_DECLARE_EXCEPTION(binary_out_of_range, object_archive_exception);

}  // namespace error

using archive::entity_type;

/**
 * dummy incomplete class type to give objects type resolution
 */
class object_data_t;
class object_metadata;  // forwarding

/**
 * Object/metadata wrapper for various use
 */
struct object_view_t
{
    object_metadata_t meta = {};
    object_data_t* data    = {};

   public:
    object_view_t() = default;
    object_view_t(const object_metadata* meta, object_data_t* data) : meta(meta), data(data) {}

    template <typename Ty_>
    object_view_t(Ty_* p) noexcept;

   public:
    auto pair() const noexcept { return std::make_pair(meta, data); }
};

struct object_const_view_t
{
    object_metadata_t meta    = {};
    object_data_t const* data = {};

   public:
    object_const_view_t() = default;
    object_const_view_t(const object_metadata* meta, const object_data_t* data) : meta(meta), data(data) {}

    template <typename Ty_>
    explicit object_const_view_t(Ty_ const& p) noexcept;

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
    object_metadata_t _meta = {};
    object_data_t* _data    = {};

   public:
    /*  Lifetime Management  */

   public:
    auto pair() const noexcept { return std::make_pair(_meta, _data); }
    operator object_view_t() noexcept { return {_meta, _data}; }
    operator object_const_view_t() noexcept { return {_meta, _data}; }
};

// alias
using object_metadata_fn = std::function<object_metadata_t()>;

/**
 * Object's sub property info
 */
struct property_metadata
{
    //! Offset from object root
    size_t offset = 0;

    //! Object descriptor for this property
    object_metadata_t type;

    //! index of self
    int index_self = 0;

    //! index key if this is property of 'object'
    int name_key_self = -1;

    //! name if this is property of 'object'
    std::string_view name;

   public:
    object_metadata_t _owner_type = {};

   public:
    auto owner_type() const { return _owner_type; }

   public:
    property_metadata() = default;
    property_metadata(
            size_t offset,
            object_metadata_fn descriptor)
            : offset(offset),
              type(descriptor()),
              _owner_type(nullptr)
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
class if_primitive_control
{
   public:
    /**
     * Return actual type of this primitive
     */
    virtual entity_type type() const noexcept = 0;

    /**
     * Underlying element type if this primitive is sort of container
     * This descriptor may not be used for actual data manipulation, but only for
     *  descriptive operations like validation/documentation/schema-generation.
     *
     * (optional, map, set, pointer, etc ...)
     */
    virtual object_metadata_t element_type() const noexcept { return nullptr; };

    /**
     * Archive to writer
     * Property may not exist if this primitive is being archived as root.
     */
    virtual void archive(archive::if_writer* strm,
                         void const* pvdata,
                         object_metadata_t desc_self,
                         optional_property_metadata opt_as_property) const
    {
        *strm << nullptr;
    }

    /**
     * Restore from reader
     * Property may not exist if this primitive is being archived as root.
     */
    virtual void restore(archive::if_reader* strm,
                         void* pvdata,
                         object_metadata_t desc_self,
                         optional_property_metadata opt_as_property) const = 0;

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

template <typename Ty_>
class templated_primitive_control : public if_primitive_control
{
   public:
    void archive(archive::if_writer* strm, const void* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const final
    {
        archive(strm, *(const Ty_*)pvdata, desc_self, opt_as_property);
    }
    void restore(archive::if_reader* strm, void* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const final
    {
        restore(strm, (Ty_*)pvdata, desc_self, opt_as_property);
    }
    requirement_status_tag status(void const* pvdata) const noexcept final
    {
        return status((Ty_ const*)pvdata);
    }

   protected:
    virtual void archive(
            archive::if_writer* strm,
            const Ty_& data,
            object_metadata_t desc_self,
            optional_property_metadata opt_as_property) const = 0;

    virtual void restore(
            archive::if_reader* strm,
            Ty_* pvdata,
            object_metadata_t desc_self,
            optional_property_metadata opt_as_property) const = 0;

    virtual requirement_status_tag
    status(const Ty_* data) const noexcept
    {
        return requirement_status_tag::required;
    }
};

/**
 * SFINAE helper for overloading get_object_metadata
 */
template <bool Test_>
using object_sfinae_t = std::enable_if_t<Test_, object_metadata_t>;

template <bool Test_>
using object_sfinae_ptr = std::enable_if_t<Test_, object_metadata_ptr>;

template <typename A_, typename B_>
using object_sfinae_overload_t = object_sfinae_t<std::is_same_v<A_, B_>>;

template <typename ValTy_, class = void>
struct get_object_metadata_t
{
};

template <typename ValTy_>
auto get_object_metadata()
{
    return get_object_metadata_t<ValTy_>{}();
}

template <typename ValTy_, class = void>
constexpr bool has_object_metadata_v = false;

template <typename ValTy_>
constexpr bool has_object_metadata_v<
        ValTy_, std::void_t<decltype(get_object_metadata<ValTy_>())>> = true;

template <typename ValTy_>
using has_object_metadata_t = std::enable_if_t<has_object_metadata_v<ValTy_>>;

/**
 * Get object descriptor getter
 */
template <typename Ty_>
object_metadata_fn default_object_metadata_fn()
{
    return [] { return get_object_metadata<Ty_>(); };
}

/**
 * Object descriptor, which can manipulate random object.
 *
 * @warning There is no way to perform dynamic type recognition only with data pointer!
 *          If you have any plan to manipulate objects without static type information,
 *           manipulate them with wrapped_object or object_pointer!
 */
class object_metadata
{
   private:
    using hierarchy_append_fn = std::function<void(object_metadata_t,
                                                   property_metadata const*)>;

   private:
    /*   Properties   */

    // extent of this object
    size_t _extent = 0;

    // property manipulator
    // if it's user defined object, it'll be nullptr.
    if_primitive_control const* _primitive = {};

    // list of properties
    // properties are not incremental in memory address, as they are
    //  simply stored in creation order.
    std::vector<property_metadata> _props;

    // [optional]
    bool _is_object = false;

    // if _is_object is true, this indicates name of properties
    sorted_vector<std::string, int> _keys;

    // if _is_object is true, this indicates set of indices which is used for fast-access key
    sorted_vector<int, int> _key_indices;

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
    bool is_object() const noexcept { return _is_object; }
    bool is_tuple() const noexcept { return not is_primitive() && not is_object(); }
    bool is_optional() const noexcept { return is_primitive() && _primitive->status() != requirement_status_tag::required; }

    entity_type type() const noexcept
    {
        if (is_primitive()) { return _primitive->type(); }
        return is_object() ? entity_type::object : entity_type::tuple;
    }

    /**
     * Current requirement status of given property
     */
    requirement_status_tag requirement_status(object_data_t const* data = nullptr) const noexcept
    {
        if (not is_primitive())
            return requirement_status_tag::required;

        return _primitive->status(data);
    }

    /**
     * Extent of this object
     */
    size_t extent() const noexcept
    {
        return _extent;
    }

    /**
     * Retrieves data from property recursively.
     */
    object_data_t* retrieve(object_data_t* data, property_metadata const& property) const;

    /**
     * Retrieves data pointer from an object
     */
    object_data_t* retrieve_self(object_data_t* data, property_metadata const& property) const
    {
        assert(_props.at(property.index_self)._owner_type == property._owner_type);
        return (object_data_t*)((char*)data + property.offset);
    }

    /**
     * Retrieves data pointer from an object
     */
    object_data_t const* retrieve_self(object_data_t const* data, property_metadata const& property) const
    {
        assert(_props.at(property.index_self)._owner_type == property._owner_type);
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
    property_metadata const* property(std::string_view key) const
    {
        if (not _is_object) { return nullptr; }  // this is not object.

        auto ptr = perfkit::find_ptr(_keys, key);
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
    void _archive_to(archive::if_writer* strm,
                     object_data_t const* data,
                     optional_property_metadata opt_property) const
    {
        // 1. iterate properties, and access to their object descriptor
        // 2. recursively call _archive_to on them.
        // 3. if 'this' is object, first add key on archive before recurse.

        if (is_primitive())
        {
            _primitive->archive(strm, data, this, opt_property);
        }
        else
        {
            if (is_object())
            {
                size_t num_filled = 0;
                for (auto& [key, index] : _keys)
                {
                    auto& prop = _props.at(index);

                    auto child      = prop.type;
                    auto child_data = retrieve_self(data, prop);
                    assert(child_data);

                    if (auto status = child->requirement_status(child_data);
                        status != requirement_status_tag::optional_empty)
                    {
                        ++num_filled;
                    }
                }

                strm->object_push(num_filled);

                auto do_write
                        = [&](auto&& key, int index) {
                              auto& prop = _props.at(index);

                              auto child      = prop.type;
                              auto child_data = retrieve_self(data, prop);
                              assert(child_data);

                              if (auto status = child->requirement_status(child_data);
                                  status == requirement_status_tag::optional_empty)
                              {
                                  return;  // skip empty optional property
                              }
                              else
                              {
                                  strm->write_key_next();
                                  *strm << key;

                                  child->_archive_to(strm, child_data, &prop);
                              }
                          };

                if (not strm->use_integer_key)
                    for (auto& [key, index] : _keys)
                        do_write(key, index);
                else
                    for (auto& [key, index] : _key_indices)
                        do_write(key, index);

                strm->object_pop();
            }
            else if (is_tuple())
            {
                strm->array_push(_props.size());
                for (auto& prop : _props)
                {
                    auto child      = prop.type;
                    auto child_data = retrieve_self(data, prop);
                    assert(child_data);

                    if (auto status = child->requirement_status(child_data);
                        status == requirement_status_tag::optional_empty)
                    {
                        // archive empty argument as null.
                        *strm << nullptr;
                    }
                    else
                    {
                        child->_archive_to(strm, child_data, &prop);
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
    };

    void _restore_from(archive::if_reader* strm,
                       object_data_t* data,
                       restore_context* context,  // for reusing key buffer during recursive
                       optional_property_metadata opt_property) const
    {
        // 1. iterate properties, and access to their object descriptor
        // 2. recursively call _restore_from on them.
        // 3. if 'this' is object, first retrieve key from archive before recurse.
        //    if target property is optional, ignore missing key

        if (is_primitive())
        {
            _primitive->restore(strm, data, this, opt_property);
        }
        else if (is_object())
        {
            if (not strm->is_object_next())
                throw error::invalid_read_state{}
                        .set(strm)
                        .message("'object' expected");

            auto context_key           = strm->begin_object();
            bool const use_integer_key = strm->use_integer_key;

            while (not strm->should_break(context_key))
            {
                int index = -1;
                strm->read_key_next();

                if (use_integer_key)
                {
                    int integer_key;
                    *strm >> integer_key;

                    auto elem = find_ptr(_key_indices, integer_key);
                    if (elem) { index = elem->second; }
                }
                else
                {
                    // retrive key, and find it from my properties list
                    auto& keybuf = context->keybuf;
                    *strm >> keybuf;

                    auto elem = find_ptr(_keys, keybuf);
                    if (elem) { index = elem->second; }
                }

                // simply ignore unexpected keys
                if (index == -1)
                {
                    nullptr_t discard = {};
                    *strm >> discard;
                    continue;
                }

                auto& prop      = _props.at(index);
                auto child      = prop.type;
                auto child_data = retrieve_self(data, prop);
                assert(child_data);

                child->_restore_from(strm, child_data, context, opt_property);
            }

            strm->end_object(context_key);
        }
        else if (is_tuple())
        {
            auto context_key = strm->begin_array();

            // tuple is always in fixed-order
            for (auto& prop : _props)
            {
                auto child = prop.type;
                if (child->is_optional() && strm->is_null_next())
                {
                    // do nothing
                    nullptr_t nul{};
                    *strm >> nul;
                    continue;
                }

                auto child_data = retrieve_self(data, prop);
                child->_restore_from(strm, child_data, context, &prop);
            }

            strm->end_array(context_key);
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
        auto descr     = property.type;

        // primitive cannot have children
        if (descr->is_primitive()) { return ~size_t{}; }

        // do recurse
        return descr->_find_property(offset - property.offset, append, ++depth);
    }

    property_metadata const* _find_property(size_t offset) const
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
        virtual ~basic_factory()                = default;
        basic_factory()                         = default;
        basic_factory(basic_factory&&) noexcept = default;
        basic_factory& operator=(basic_factory&&) noexcept = default;

       protected:
        std::unique_ptr<object_metadata> _current
                = std::make_unique<object_metadata>();

       protected:
        size_t add_property_impl(property_metadata info)
        {
            assert(not _current->is_primitive());

            auto index       = _current->_props.size();
            auto prop        = &_current->_props.emplace_back(info);
            prop->index_self = index;

            return index;
        }

       public:
        /**
         * Generate object descriptor instance.
         * Sorts keys, build incremental offset lookup table, etc.
         */
        object_metadata_ptr create()
        {
            static std::atomic_uint64_t unique_id_allocator;

            auto result     = std::move(_current);
            auto& generated = *result;
            auto lookup     = &generated._offset_lookup;

            lookup->reserve(generated._props.size());

            // for name key autogeneration
            std::vector<int> used_name_keys;

            bool const is_object = generated.is_object();
            used_name_keys.reserve(generated._props.size());

#ifndef NDEBUG
            size_t object_end = 0;
#endif
            for (auto& prop : generated._props)
            {
                lookup->emplace_back(std::make_pair(prop.offset, prop.index_self));
                prop._owner_type = result.get();

                if (is_object)
                {
                    if (prop.name_key_self == 0)
                        throw std::logic_error{"name key must be larger than 0!"};
                    if (prop.name_key_self > 0)
                    {
                        used_name_keys.push_back(prop.name_key_self);
                        push_heap(used_name_keys, std::greater<>{});
                    }
                }

#ifndef NDEBUG
                object_end = std::max(object_end, prop.offset + prop.type->extent());
#endif
            }

            // assign name_key table
            if (is_object)
            {
                if (adjacent_find(used_name_keys) != used_name_keys.end())
                    throw std::logic_error("duplicated name key assignment found!");

                // target key table
                generated._key_indices.reserve(generated._props.size());

                size_t idx_table    = 0;
                int generated_index = 1;
                for (auto& prop : generated._props)
                {
                    // autogenerate unassigned ones
                    if (prop.name_key_self < 0)
                    {
                        // skip all already pre-assigned indices
                        while (not used_name_keys.empty() && used_name_keys.front() <= generated_index)
                        {
                            generated_index = used_name_keys.front() + 1;

                            pop_heap(used_name_keys, std::greater<>{});
                            used_name_keys.pop_back();
                        }

                        prop.name_key_self = generated_index++;
                    }

                    auto const is_unique
                            = generated
                                      ._key_indices
                                      .try_emplace(prop.name_key_self, prop.index_self)
                                      .second;

                    (void)is_unique;  // prevent warning on NDEBUG
                    assert(is_unique);
                }
            }

            // simply sort incrementally.
            std::sort(lookup->begin(), lookup->end());

            assert("Offset must not duplicate"
                   && adjacent_find(
                              *lookup, [](auto& a, auto& b) {
                                  return a.first == b.first;
                              })
                              == lookup->end());

            assert("End of address must be less or equal with actual object extent"
                   && object_end <= generated.extent());

            generated._initialized = true;  // set initialized
            return result;
        }
    };

    class primitive_factory : public basic_factory
    {
       public:
        static object_metadata_ptr
        define(size_t extent, if_primitive_control const* static_ref_prm_ctrl)
        {
            return primitive_factory{}.setup(extent, static_ref_prm_ctrl).create();
        }

       private:
        primitive_factory& setup(size_t extent, if_primitive_control const* ref)
        {
            *_current = {};

            _current->_extent    = extent;
            _current->_primitive = ref;

            return *this;
        }
    };

    template <typename Ty_>
    class object_factory_base : public basic_factory
    {
       private:
        Ty_ const& _self() const
        {
            return static_cast<Ty_ const&>(*this);
        }

       protected:
        template <typename Class_>
        Ty_ const& define_class()
        {
            define_basic(sizeof(Class_));

            // TODO: add construct/move/copy/invoke manipulators ...
            //       DESIGN HOW TO DO IT!

            return _self();
        }

        virtual Ty_ const& define_basic(size_t extent)
        {
            *_current         = {};
            _current->_extent = extent;

            return _self();
        }

        template <typename Class_, typename MemVar_>
        static property_metadata create_property_metadata(MemVar_ Class_::*mem_ptr)
        {
            property_metadata info;
            info.type   = get_object_metadata<MemVar_>();
            info.offset = reinterpret_cast<size_t>(
                    &(reinterpret_cast<Class_ const volatile*>(NULL)->*mem_ptr));

            return info;
        }

        template <typename Class_, typename MemVar_>
        static property_metadata create_property_metadata(Class_ const* class_type, MemVar_ const* mem_ptr)
        {
            return create_property_metadata<Class_, MemVar_>();
        }
    };

    class object_factory : public object_factory_base<object_factory>
    {
        using super = object_factory_base<object_factory>;

       protected:
        object_factory const& define_basic(size_t extent) override
        {
            object_factory_base::define_basic(extent);
            _current->_is_object = true;

            // hard coded object size estimation ...
            _current->_keys.reserve(32);

            return *this;
        }

        auto& add_property(std::string key, property_metadata info)
        {
            auto index          = add_property_impl(std::move(info));
            auto [pair, is_new] = _current->_keys.try_emplace(std::move(key), int(index));
            // _current->_key_indices: delay until key evaluation on create()

            if (not is_new) { throw std::logic_error{"Key must be unique"}; }

            // register property name
            _current->_props[index].name = pair->first;

            return *this;
        }

        auto& basic_extend(object_metadata_t meta, size_t base_offset)
        {
            if (not meta->is_object()) { throw std::logic_error{"Non-object cannot be derived"}; }
            // TODO: add metadata for representing base object itself, for dynamic usage?

            for (auto prop : meta->_props)
            {
                prop.offset += base_offset;
                add_property(std::string{prop.name}, prop);
            }

            return *this;
        }
    };

    class tuple_factory : public object_factory_base<tuple_factory>
    {
        using super = object_factory_base<object_factory>;

       protected:
        auto& add_property(property_metadata info)
        {
            add_property_impl(std::move(info));

            return *this;
        }
    };

    template <class Class_>
    class templated_object_factory : public object_factory
    {
       public:
        static templated_object_factory define()
        {
            templated_object_factory factory;
            factory.define_class<Class_>();
            return factory;
        }

        template <typename MemVar_>
        auto& property(MemVar_ Class_::*mem_ptr, std::string name, int name_key = -1)
        {
            auto info          = create_property_metadata(mem_ptr);
            info.name_key_self = name_key;
            add_property(std::move(name), std::move(info));
            return *this;
        }

        template <typename MemVar_>
        auto& _property_3(MemVar_ Class_::*mem_ptr, std::string name, int name_key = -1)
        {
            return property(mem_ptr, std::move(name), -1);
        }

        template <typename KeyStr_, typename MemVar_>
        auto& _property_3(MemVar_ Class_::*mem_ptr, KeyStr_&&, std::string name, int name_key = -1)
        {
            return property(mem_ptr, std::move(name), name_key);
        }

        template <typename Base_, typename = std::enable_if_t<std::is_base_of_v<Base_, Class_>>>
        auto& extend()
        {
            constexpr Class_* pointer = nullptr;
            const auto offset         = (intptr_t) static_cast<Base_*>(pointer) - (intptr_t)pointer;

            return basic_extend(get_object_metadata<Base_>(), offset);
        }
    };

    template <class Class_>
    class template_tuple_factory : public tuple_factory
    {
       public:
        static template_tuple_factory define()
        {
            template_tuple_factory factory;
            factory.define_class<Class_>();
            return factory;
        }

        template <typename MemVar_>
        auto& property(MemVar_ Class_::*mem_ptr)
        {
            add_property(create_property_metadata(mem_ptr));
            return *this;
        }
    };
};

template <typename Class_>
auto define_object()
{
    return object_metadata::templated_object_factory<Class_>::define();
}

template <typename Class_>
auto define_tuple()
{
    return object_metadata::template_tuple_factory<Class_>::define();
}

/*
 * User type definition
 */
namespace detail {
template <typename Ty_, class = void>
constexpr bool is_cpph_refl_object_v = false;

template <typename Ty_>
constexpr bool is_cpph_refl_object_v<
        Ty_, std::void_t<decltype(std::declval<Ty_>().initialize_object_metadata())>> = true;
}  // namespace detail

template <typename ValTy_>
struct get_object_metadata_t<ValTy_, std::enable_if_t<detail::is_cpph_refl_object_v<ValTy_>>>
{
    auto operator()() const;
};

}  // namespace CPPHEADERS_NS_::refl

/**
 * Implmentations
 */
namespace CPPHEADERS_NS_::refl {

/**
 * A dummy template to identify type safely
 */
template <typename ValTy_>
struct type_tag
{
};

template <typename ValTy_>
constexpr type_tag<ValTy_> type_tag_v = {};

namespace detail {
template <typename ValTy_>
auto initialize_object_metadata(type_tag<ValTy_>)
        -> object_sfinae_t<std::is_same_v<void, ValTy_>>;

struct initialize_object_metadata_fn
{
    template <typename ValTy_>
    auto operator()(type_tag<ValTy_>) const
            -> decltype(initialize_object_metadata(type_tag_v<ValTy_>))
    {
        return initialize_object_metadata(type_tag_v<ValTy_>);
    }
};
}  // namespace detail

namespace {
static constexpr auto initialize_object_metadata
        = static_const<detail::initialize_object_metadata_fn>::value;
}

template <typename ValTy_>
struct get_object_metadata_t<
        ValTy_,
        std::enable_if_t<std::is_same_v<
                object_metadata_ptr, decltype(initialize_object_metadata(type_tag_v<ValTy_>))>>>
{
    auto operator()() const
    {
        static auto instance = initialize_object_metadata(type_tag_v<ValTy_>);
        return &*instance;
    }
};

template <typename ValTy_, class = void>
constexpr bool has_object_metadata_initializer_v = false;

template <typename ValTy_>
constexpr bool has_object_metadata_initializer_v<
        ValTy_, std::void_t<decltype(initialize_object_metadata(type_tag_v<ValTy_>))>> = true;

template <typename Ty_>
object_view_t::object_view_t(Ty_* p) noexcept : data((object_data_t*)p)
{
    meta = get_object_metadata<Ty_>();
}

template <typename Ty_>
object_const_view_t::object_const_view_t(const Ty_& p) noexcept : data((object_data_t const*)&p)
{
    meta = get_object_metadata<Ty_>();
}

}  // namespace CPPHEADERS_NS_::refl

namespace std {
inline std::string to_string(CPPHEADERS_NS_::refl::entity_type t)
{
    switch (t)
    {
        case CPPHEADERS_NS_::refl::entity_type::invalid: return "invalid";
        case CPPHEADERS_NS_::refl::entity_type::null: return "null";
        case CPPHEADERS_NS_::refl::entity_type::boolean: return "boolean";
        case CPPHEADERS_NS_::refl::entity_type::string: return "string";
        case CPPHEADERS_NS_::refl::entity_type::binary: return "binary";
        case CPPHEADERS_NS_::refl::entity_type::dictionary: return "dictionary";
        case CPPHEADERS_NS_::refl::entity_type::array: return "array";
        case CPPHEADERS_NS_::refl::entity_type::integer: return "integer";
        case CPPHEADERS_NS_::refl::entity_type::floating_point: return "floating_point";
        case CPPHEADERS_NS_::refl::entity_type::object: return "object";
        case CPPHEADERS_NS_::refl::entity_type::tuple: return "tuple";
        default: return "__NONE__";
    }
}
}  // namespace std

namespace CPPHEADERS_NS_::archive {

/**
 * Dump object to archive
 */
template <>
inline if_writer&
operator<<(if_writer& writer, refl::object_const_view_t const& value)
{
    value.meta->_archive_to(&writer, value.data, nullptr);
    return writer;
}

/**
 * Restore object from archive
 */
inline if_reader&
operator>>(if_reader& strm, refl::object_view_t const& obj)
{
    refl::object_metadata::restore_context ctx;
    obj.meta->_restore_from(&strm, obj.data, &ctx, nullptr);
    return strm;
}

template <>
inline if_reader&
operator>>(if_reader& strm, refl::object_view_t& obj)
{
    return strm >> (const refl::object_view_t&)obj;
}

template <typename ValTy_>
if_writer& if_writer::serialize(const ValTy_& in)
{
    refl::object_const_view_t view{in};
    return *this << view;
}

template <typename Ty_>
if_writer& if_writer::write(const Ty_& other)
{
    return serialize(other);
}

template <typename ValTy_>
if_reader& if_reader::deserialize(ValTy_& out)
{
    refl::object_view_t view{&out};
    return *this >> view;
}

template <typename Ty_>
if_reader& if_reader::read(Ty_& other)
{
    return deserialize(other);
}

}  // namespace CPPHEADERS_NS_::archive
