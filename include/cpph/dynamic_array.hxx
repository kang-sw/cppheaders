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

#include "utility/array_view.hxx"

//

namespace cpph {
template <typename Ty_>
class dynamic_array : public array_view<Ty_>
{
   public:
    dynamic_array(size_t num_elems)  //
            noexcept(std::is_nothrow_default_constructible_v<Ty_>)
            : array_view<Ty_>(new Ty_[num_elems], num_elems),
              _buf(this->data())
    {
    }

    dynamic_array() noexcept = default;
    dynamic_array(dynamic_array&& other) noexcept { *this = std::move(other); }
    dynamic_array& operator=(dynamic_array&& other) noexcept
    {
        this->array_view<Ty_>::operator=(other);
        _buf = std::move(other._buf);

        other.array_view<Ty_>::operator=({});
        return *this;
    }

    void resize(size_t new_size)
    {
        *this = dynamic_array<Ty_>{new_size};
    }

   private:
    std::unique_ptr<Ty_[]> _buf = nullptr;
};

}  // namespace cpph