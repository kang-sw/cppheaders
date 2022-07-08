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
#include <list>

#include "cpph/utility/generic.hxx"

namespace cpph {
using std::list;

template <typename T>
struct _list_pool_base_1 {
    using self_type = _list_pool_base_1;
    list<T> _body;

    _list_pool_base_1() noexcept = default;

    list<T>* _get_ptr() noexcept { return &_body; }
    list<T> const* _get_ptr() const noexcept { return &_body; }
};

template <typename T>
struct _list_pool_base_2 {
    using self_type = _list_pool_base_2;
    list<T>* _pool;

    explicit _list_pool_base_2(list<T>* pool) noexcept : _pool(pool) {}
    list<T>* _get_ptr() noexcept { return _pool; }
    list<T> const* _get_ptr() const noexcept { return _pool; }
};

template <typename T, bool UseExisting>
class basic_list_pool : conditional_t<UseExisting, _list_pool_base_1<T>, _list_pool_base_2<T>>
{
   public:
    using owner_type = typename basic_list_pool::self_type;
    using list_type = list<T>;

    // Inherit constructor
    using owner_type::owner_type;

   public:
    void checkin(list_type* other)
    {
        list_type* ptr = _ptr();
        ptr->splice(ptr->end(), *other);
    }

    void checkin(list_type* other, typename list_type::iterator iter)
    {
        list_type* ptr = _ptr();
        ptr->splice(ptr->end(), *other, iter);
    }

    void checkin(list_type* other,
                 typename list_type::iterator begin,
                 typename list_type::iterator end)
    {
        list_type* ptr = _ptr();
        ptr->splice(ptr->end(), *other, begin, end);
    }

    auto checkout(list_type* other, typename list_type::iterator where)
            -> typename list_type::iterator
    {
        list_type* ptr = _ptr();
        if (auto iter = ptr->begin(); iter != ptr->end()) {
            return other->splice(where, *ptr, iter), iter;
        } else {
            return other->emplace(where);
        }
    }

    T& checkout_back(list_type* other)
    {
        return *checkout(other, other->end());
    }

    T& checkout_front(list_type* other)
    {
        return *checkout(other, other->begin());
    }

   private:
    list_type* _ptr() noexcept { return this->_get_ptr(); }
    list_type const* _ptr() const noexcept { return this->_get_ptr(); }
};

template <typename T>
using list_pool = basic_list_pool<T, true>;

template <typename T>
using borrowed_list_pool = basic_list_pool<T, false>;

}  // namespace cpph
