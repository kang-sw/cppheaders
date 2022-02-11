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

constexpr inline bool _is_pow2(size_t value)
{
    size_t n_one = 0;
    for (; value > 0; value >>= 1) { n_one += value & 1; }
    return n_one == 1;
}
}  // namespace detail

template <typename Ty_,
          size_t BlockSize_ = size_t{1} << detail::_nearlest_llog2(1024 / sizeof(Ty_) * 2 - 1),
          typename Alloc_   = std::allocator<Ty_>>
class deque : Alloc_  // zero_size optimization
{
    // block size must be power of 2 for optimization
    static_assert(detail::_is_pow2(BlockSize_));

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
        _dt_triv     = std::is_trivially_destructible_v<value_type>,

        _shift = detail::_nearlest_llog2(block_size),
        _mask  = block_size - 1,
    };

   public:
    template <bool IsMutable_>
    class _basic_iterator
    {
        using container_type = std::conditional_t<IsMutable_, deque, deque const>;

        deque* _owner;
        size_t _index;

       private:
        void _step_once() noexcept
        {
        }
    };

    using iterator       = _basic_iterator<true>;
    using const_iterator = _basic_iterator<false>;

   private:
    std::vector<_block_type*> _buffers;

    size_type _ofst;  // offset from first array element
    size_type _size;

   public:
    auto empty() const noexcept { return _size == 0; }
    auto size() const noexcept { return _size; }
    auto capacity() const noexcept { return _n_blocks() * block_size; }

    void reserve(size_type n_elems)
    {
        auto n_new_blk = _calc_n_blocks(n_elems);
        auto n_cur_blk = _n_blocks();
        if (n_new_blk > n_cur_blk) { return; }  // do nothing if already sufficient

        n_new_blk -= n_cur_blk;
        _buffers.reserve(_buffers.size() + n_new_blk);
        while (n_new_blk--) { _alloc_once(); }
    }

    void shrink_to_fit() noexcept
    {
    }

    // NOTE: Insertion
    //  - spin until element size exceeds capacity.
    //    once exceeds, reallocate pointer buffer and make them in right order.
    //  - during spin, efficiency between inserting back and front are similar.
    //    however, expanding front is less efficient than back, as it requires
    //     rearrangement of buffer pointer array on every buffer growth.
    template <typename... Args_>
    reference emplace_back() noexcept(std::is_nothrow_constructible_v<value_type, Args_...>)
    {
        _reserve_back(1);
    }

    template <typename... Args_>
    reference emplace_front() noexcept(std::is_nothrow_constructible_v<value_type, Args_...>)
    {
        _reserve_front(1);
    }

   private:
    void _reserve_back(size_t n)
    {
        if (_space_head() < n)
        {
        }
    }

    void _reserve_front(size_t n)
    {
    }

    void _flatten()
    {
        // make deque buffers be flatten
    }

    bool _is_block_full() const noexcept
    {
    }

    size_t _space_head() const noexcept
    {
        // remaining space that can be used for inserting elements into head.
        // (number of fully empty blocks + remining size of current block)
        auto [min, max] = std::minmax(_blk_tail(), _blk_head());
        auto n_empty    = (max - min) * block_size;

        return 0;
    }

    size_t _space_tail() const noexcept
    {
        // remaining space that can be used for inserting elements into tail
        // (number of fully empty blocks + remining size of current block)
        auto cur_block_occupied = block_size - (_ofst & _mask);

        return block_size * (_blk_head_linear() - _blk_tail()) + cur_block_occupied;
    }

    size_t _blk_tail() const noexcept
    {
        return _ofst >> _shift;
    }

    size_t _blk_head() const noexcept
    {
        return _blk_head_linear() % _buffers.size();
    }

    size_t _blk_head_linear() const noexcept
    {
        return ((_ofst + _size) >> _shift);
    }

    size_type _n_blocks() const noexcept
    {
        return _buffers.size();
    }

    static size_type _calc_n_blocks(size_type n_elems) noexcept
    {
        return (n_elems + _mask) >> _shift;
    }

    void _alloc_once()
    {
        _buffers.push_back(reinterpret_cast<_block_type*>(this->allocate(block_size)));
    }
};

static_assert(deque<int>::block_size == 256);
static_assert(deque<double>::block_size == 128);
static_assert(deque<std::array<char, 511>>::block_size == 2);
static_assert(deque<std::array<char, 512>>::block_size == 2);
static_assert(deque<std::array<char, 513>>::block_size == 1);
static_assert(deque<std::array<char, 10041>>::block_size == 1);
}  // namespace CPPHEADERS_NS_
