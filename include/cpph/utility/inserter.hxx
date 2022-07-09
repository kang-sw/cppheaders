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

#include "generic.hxx"

namespace cpph {
template <typename InsertFn_>
class insert_adapter
{
   public:
    InsertFn_ assign;

   public:
    insert_adapter(InsertFn_ fn) noexcept : assign(std::move(fn)) {}

    insert_adapter& operator*() noexcept { return *this; };
    insert_adapter& operator++(int) noexcept { return *this; }
    insert_adapter& operator++() noexcept { return *this; }

    template <typename Ty_, class = std::enable_if_t<not std::is_same_v<Ty_, insert_adapter>>>
    insert_adapter& operator=(Ty_&& other)
    {
        assign(std::forward<Ty_>(other));
        return *this;
    }
};

template <typename InsertFn_>
insert_adapter(InsertFn_&&) -> insert_adapter<InsertFn_>;

}  // namespace cpph