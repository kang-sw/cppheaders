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

#include <nlohmann/json_fwd.hpp>

#include "cpph/refl/detail/primitives.hxx"

namespace cpph::refl::_detail {
void _archive_recursive(archive::if_writer* strm, const nlohmann::json& data);
void _restore_recursive(archive::if_reader* strm, nlohmann::json* pdata, string* keybuf);
}  // namespace cpph::refl::_detail

//
#include "../detail/_init_macros.hxx"
CPPH_REFL_DEF_begin(JsonLike, std::is_same_v<JsonLike, nlohmann::json>)
{
    CPPH_REFL_DEF_type(object);
    CPPH_REFL_DEF_archive(strm, data)
    {
        cpph::refl::_detail::_archive_recursive(strm, data);
    }
    CPPH_REFL_DEF_restore(strm, data)
    {
        std::string keybuf;
        cpph::refl::_detail::_restore_recursive(strm, data, &keybuf);
    }
}
CPPH_REFL_DEF_end();
#include "../detail/_deinit_macros.hxx"