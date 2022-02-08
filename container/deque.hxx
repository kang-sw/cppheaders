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
#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "__namespace__"
#include "buffer.hxx"

namespace CPPHEADERS_NS_ {
namespace detail {
constexpr inline size_t _nearlest_llog2(size_t value)
{
    if (value == ~size_t{}) { return 0; }

    size_t n = 0;
    for (; value > 1; value >>= 1) { ++n; }

    return n;
}
}  // namespace detail

template <typename Ty_,
          size_t BlockSize_ = size_t{1} << detail::_nearlest_llog2(1024 / sizeof(Ty_) * 2 - 1),
          typename Alloc_   = std::allocator<Ty_>>
class deque : Alloc_  // zero_size optimization
{
   public:
    using value_type      = Ty_;
    using reference       = Ty_&;
    using const_reference = Ty_ const&;
    using pointer         = Ty_*;
    using const_pointer   = Ty_ const*;
    using allocator_type  = Alloc_;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    enum : size_t
    {
        block_size = BlockSize_
    };

   private:
    using _block_type = std::array<Ty_, block_size>;

    enum
    {
        _ct_df_trive = std::is_trivially_default_constructible_v<value_type>,
        _cp_triv     = std::is_trivially_copyable_v<value_type>,
        _dt_triv     = std::is_trivially_destructible_v<value_type>
    };

   public:
    template <bool IsMutable_>
    class _basic_iterator
    {
        using container_type = std::conditional_t<IsMutable_, deque, deque const>;

        _block_type** _base;
        _block_type* _head;

       private:
        void _step_once() noexcept
        {
            if (++_head - *_base < block_size)
                ;
            else
                ++_base, _head = *_base ? *_base : nullptr;
        }
    };

    using iterator       = _basic_iterator<true>;
    using const_iterator = _basic_iterator<false>;

   private:
    std::vector<_block_type*> _in_use;
    std::vector<_block_type*> _in_avail;

    size_type _ofst;  // offset from first array element
    size_type _size;

   public:
    auto empty() const noexcept { return _size == 0; }
    auto size() const noexcept { return _size; }
    auto capacity() const noexcept { return _n_blocks() * block_size; }

   private:
    size_t _n_blocks() const noexcept
    {
        return (_in_use.empty() ? 0 : _in_use.size() - 1) + _in_avail.size();
    }

    void _reserve_blocks(size_t n_total)
    {
        if (n_total >= _n_blocks()) { return; }
        n_total -= _n_blocks();

        _in_avail.reserve(_in_avail.size() + n_total);

        while (n_total--)
            _in_avail.push_back(reinterpret_cast<_block_type*>(this->allocate(block_size)));
    }
};

static_assert(deque<int>::block_size == 256);
static_assert(deque<double>::block_size == 128);
static_assert(deque<std::array<char, 511>>::block_size == 2);
static_assert(deque<std::array<char, 512>>::block_size == 2);
static_assert(deque<std::array<char, 513>>::block_size == 1);
}  // namespace CPPHEADERS_NS_
