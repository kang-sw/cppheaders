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
#include <stdexcept>
#include <type_traits>

#include "../__namespace__"
#include "../template_utils.hxx"

namespace CPPHEADERS_NS_ {
template <typename InsertFn_>
class adapt_insert_iterator
{
   public:
    InsertFn_ assign;

   public:
    adapt_insert_iterator(InsertFn_ fn) noexcept : assign(std::move(fn)) {}

    adapt_insert_iterator& operator*() noexcept { return *this; };
    adapt_insert_iterator& operator++(int) noexcept { return *this; }
    adapt_insert_iterator& operator++() noexcept { return *this; }

    template <typename Ty_, class = std::enable_if_t<not std::is_same_v<Ty_, adapt_insert_iterator>>>
    adapt_insert_iterator& operator=(Ty_&& other)
    {
        assign(std::forward<Ty_>(other));
        return *this;
    }
};

template <typename InsertFn_>
auto adapt_inserter(InsertFn_&& insert_fn)
{
    return adapt_insert_iterator<remove_cvr_t<InsertFn_>>{std::forward<InsertFn_>(insert_fn)};
}

}  // namespace CPPHEADERS_NS_