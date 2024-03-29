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
#include <type_traits>
#include <utility>

namespace cpph {
template <typename Callable>
class cleanup_t
{
   public:
    // NOTE: As cleanup process may called during stack unrolling, internal callable MAY NOT THROW
    //        exception !
    ~cleanup_t() noexcept(std::is_nothrow_invocable_v<Callable>) { _callable(); }
    Callable _callable;
};

template <typename Callable>
cleanup_t(Callable) -> cleanup_t<Callable>;

template <typename Callable>
[[maybe_unused]] auto cleanup(Callable&& callable)
{
    return cleanup_t<std::remove_reference_t<Callable>>{
            std::forward<Callable>(callable)};
}
}  // namespace cpph