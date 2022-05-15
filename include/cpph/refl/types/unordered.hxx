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
#include <unordered_map>
#include <unordered_set>

#include "../detail/primitives.hxx"
//
#include "../detail/_init_macros.hxx"

namespace cpph {
using std::unordered_map;
using std::unordered_set;
}  // namespace cpph

namespace cpph::refl {
INTERNAL_CPPH_define_(ValTy_, (is_template_instance_of<ValTy_, std::unordered_set>::value))
{
    return _detail::get_list_like_descriptor<ValTy_>();
}
INTERNAL_CPPH_define_(ValTy_, (is_template_instance_of<ValTy_, std::unordered_map>::value))
{
    return _detail::get_dictionary_descriptor<ValTy_>();
}
}  // namespace cpph::refl

#include "../detail/_deinit_macros.hxx"