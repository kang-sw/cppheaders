
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
#include <optional>
#include <string_view>

#include "../template_utils.hxx"
#include "archive.hxx"

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_::archive {

class if_property_manipulator
{
   public:
    virtual ~if_property_manipulator() = default;

   public:
    virtual void serialize(void const*, class if_serializer*) const = 0;
};

struct property
{
    size_t offset = 0;
    size_t size   = 0;

    if_property_manipulator* manip = nullptr;
};

class object_descriptor
{
   public:
   private:
    friend class object_factory;
    friend class array_factory;

    // if keys exist, this object indicate map. otherwise, array.
    std::unique_ptr<std::string[]> _keys;

    // list of properties
    std::vector<property> _props;
};

class object_factory
{
   private:
    object_factory& _add_property(property new_property)
    {
        _props.push_back(new_property);
        return *this;
    }

   public:
    template <typename Ty_>
    object_factory& add_property(size_t offset, size_t size)
    {
        property prop = {};
        prop.offset   = offset;
        prop.size     = size;

        class manip : public if_property_manipulator
        {
           public:
            void serialize(const void* p, struct if_serializer* serializer) const override
            {
                serialize_property(*(Ty_ const*)p, serializer);
            }
        };

        prop.manip = &singleton<manip>::get();
        return _add_property(prop);
    }

   private:
    std::vector<property> _props;
};

class array_factory
{

};

template <typename Ty_>
class object_descriptor_marker;

template <typename Ty_>
decltype(std::declval<std::enable_if_t<std::is_arithmetic_v<Ty_>>>())
serialize_property(Ty_ const& p, class if_serializer* serl)
{
    if constexpr (std::is_integral_v<Ty_>)
    {
        if constexpr (sizeof(Ty_) == 1)
            serl->add_int8(p);
        else if constexpr (sizeof(Ty_) == 2)
            serl->add_int16(p);
        else if constexpr (sizeof(Ty_) == 4)
            serl->add_int32(p);
        else if constexpr (sizeof(Ty_) == 8)
            serl->add_int64(p);
        else
            serl->add_binary(&p, sizeof p);
    }
    else if constexpr (std::is_floating_point_v<Ty_>)
    {
        if constexpr (sizeof(Ty_) == 4)
            serl->add_float(p);
        else
            serl->add_double(p);
    }
}

template <typename Ty_>
decltype(std::declval<object_descriptor_marker<Ty_>>())
serialize_property(Ty_ const& p, class if_serializer* serl)
{
}

}  // namespace CPPHEADERS_NS_::archive