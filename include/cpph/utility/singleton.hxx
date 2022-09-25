
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
#include <atomic>
#include <cassert>
#include <cstddef>
#include <type_traits>

namespace cpph {
template <typename Ty_, typename Label_ = void>
struct singleton_t {
   private:
    static Ty_*& _ptr() noexcept
    {
        static Ty_* pointer;
        return pointer;
    }

   public:
    template <typename... Args_>
    nullptr_t create(Args_&&... args) const noexcept(std::is_nothrow_constructible_v<Ty_, Args_...>)
    {
        assert(not _ptr());
        _ptr() = new Ty_(std::forward<Args_>(args)...);
        return nullptr;
    }

    template <typename = void>
    void destroy() const noexcept(std::is_nothrow_destructible_v<Ty_>)
    {
        assert(_ptr());
        delete _ptr();
        _ptr() = nullptr;
    }

    /**
     * Returns default-constructed instance
     */
    Ty_& get() const noexcept
    {
        assert(_ptr());
        return *_ptr();
    }

    Ty_* operator->() const noexcept { return &get(); }
    Ty_& operator*() const noexcept { return get(); }
};

template <typename Ty_, typename Label_ = void>
constexpr inline static singleton_t<Ty_, Label_> singleton = {};

template <typename Ty_, typename Label_ = void>
Ty_& default_singleton()
{
    static Ty_ _instance;
    return _instance;
}

template <class T, class Label = void>
struct basic_singleton {
    using value_type = T;
    static T& get() noexcept { return default_singleton<T, Label>(); }
};

}  // namespace cpph
