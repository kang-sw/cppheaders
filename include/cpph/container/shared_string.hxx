/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2021-2022. Seungwoo Kang
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
#include <memory>
#include <cpph/std/string>
#include <cpph/std/string_view>

//

namespace cpph {
template <typename CharTy_, typename Traits_, typename Alloc_>
class basic_shared_string
{
   public:
    using string_type = std::basic_string<CharTy_, Traits_, Alloc_>;
    using string_view_type = std::basic_string_view<CharTy_, Traits_>;

   public:
    basic_shared_string() noexcept = default;

    template <size_t N_>
    basic_shared_string(CharTy_ const (&str)[N_]) noexcept
            : _string(std::make_shared<string_type>(str))
    {
    }

    basic_shared_string(CharTy_ const* str) noexcept
            : _string(std::make_shared<string_type>(str)) {}

    basic_shared_string(string_type str) noexcept
            : _string(std::make_shared<string_type>(std::move(str))) {}

    explicit basic_shared_string(string_view_type str) noexcept
            : _string(std::make_shared<string_type>(str)) {}

    basic_shared_string(basic_shared_string const& r) noexcept = default;
    basic_shared_string(basic_shared_string&& r) noexcept = default;
    basic_shared_string& operator=(basic_shared_string const& r) noexcept = default;
    basic_shared_string& operator=(basic_shared_string&& r) noexcept = default;

    template <typename Str_,
              typename std::enable_if_t<std::is_convertible_v<Str_, string_type>> Require_ = nullptr>
    basic_shared_string& operator=(Str_&& r) noexcept
    {
        if (_string && *_string == r) { return *this; }  // Do not copy if identical
        _string = std::make_shared<string_type>(std::forward<Str_>(r));
    };

   public:
    std::string const& string() const noexcept { return *_string; }
    bool is_valid() const noexcept { return _string; }
    operator bool() const noexcept { return is_valid(); }

    operator string_type const&() const noexcept { return *_string; }
    operator string_view_type() const noexcept { return *_string; };
    std::string const& operator*() const noexcept { return *_string; }
    std::string const* operator->() const noexcept { return &*_string; }

    size_t size() const noexcept { return _string->size(); }
    bool empty() const noexcept { return _string->empty(); }
    char const* c_str() const noexcept { return _string->c_str(); }

    auto begin() noexcept { return _string->begin(); }
    auto end() noexcept { return _string->end(); }
    auto rbegin() noexcept { return _string->rbegin(); }
    auto rend() noexcept { return _string->rend(); }

    auto begin() const noexcept { return _string->begin(); }
    auto end() const noexcept { return _string->end(); }
    auto cbegin() const noexcept { return _string->cbegin(); }
    auto cend() const noexcept { return _string->cend(); }
    auto rbegin() const noexcept { return _string->rbegin(); }
    auto rend() const noexcept { return _string->end(); }

   private:
    std::shared_ptr<string_type>
            _string;
};

template <typename CharTy_,
          typename Traits_ = std::char_traits<CharTy_>,
          typename Alloc_ = std::allocator<CharTy_>>
class basic_shared_string;

using shared_string = basic_shared_string<char>;
using wshared_string = basic_shared_string<wchar_t>;

}  // namespace cpph
