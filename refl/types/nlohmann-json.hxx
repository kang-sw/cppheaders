/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2022. Seungwoo Kang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * project home: https://github.com/perfkitpp
 ******************************************************************************/

#pragma once
#include <list>

#include <nlohmann/json.hpp>

#include "../detail/primitives.hxx"

//
#include "../detail/_init_macros.hxx"

namespace CPPHEADERS_NS_::refl {
namespace detail {
class nlohmann_json_manip_t : public templated_primitive_control<nlohmann::json>
{
   public:
    entity_type type() const noexcept override
    {
        return entity_type::string;
    }

   protected:
    void impl_archive(archive::if_writer* strm, const nlohmann::json& data, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
    {
        _recurse_archive(strm, data);
    }

    void impl_restore(archive::if_reader* strm, nlohmann::json* pvdata, object_metadata_t desc_self, optional_property_metadata opt_as_property) const override
    {
        _recurse_restore(strm, pvdata);
    }

   private:
    void _recurse_archive(archive::if_writer* strm, const nlohmann::json& data) const
    {
        // TODO

        for (auto& [key, value] : data.items()) {
        }
    }

    void _recurse_restore(archive::if_reader* strm, nlohmann::json* pdata) const
    {
    }
};
}  // namespace detail

INTERNAL_CPPH_define_(ValTy_, (std::is_same_v<ValTy_, nlohmann::json>))
{}
}  // namespace CPPHEADERS_NS_::refl

#include "../detail/_deinit_macros.hxx"