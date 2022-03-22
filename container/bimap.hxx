/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2022. Seungwoo Kang
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

//
// Created by ki608 on 2022-03-22.
//

#pragma once
#include <cassert>
#include <map>
#include <vector>

#include "__namespace__"

namespace CPPHEADERS_NS_ {
template <typename Key1, typename Key2, bool UniqueSecondKey>
class bimap
{
   public:
    using value_type = std::pair<Key1, Key2>;
    using container_type = std::vector<value_type>;
    using second_map_type = std::map<Key2, size_t, std::less<>>;
    using first_map_type = std::map<Key1, size_t, std::less<>>;

    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;
    using reverse_iterator = typename container_type::reverse_iterator;
    using const_reverse_iterator = typename container_type::const_reverse_iterator;

   private:
    container_type  _data;
    first_map_type  _first;
    second_map_type _second;

   public:
    auto begin() noexcept { return _data.begin(); }
    auto begin() const noexcept { return _data.begin(); }
    auto cbegin() const noexcept { return _data.begin(); }

    auto end() noexcept { return _data.end(); }
    auto end() const noexcept { return _data.end(); }
    auto cend() const noexcept { return _data.end(); }

    auto rbegin() noexcept { return _data.rbegin(); }
    auto rbegin() const noexcept { return _data.rbegin(); }

    auto rend() noexcept { return _data.rend(); }
    auto rend() const noexcept { return _data.rend(); }

    auto size() const noexcept { return _data.size(); }
    auto empty() const noexcept { return _data.empty(); }

    void shrink_to_fit() { _data.shrink_to_fit(); }
    void reserve(size_t n) { _data.reserve(n); }
    void clear() { _data.clear(), _first.clear(), _second.clear(); }

    bimap() noexcept = default;

    bimap(std::initializer_list<value_type> init)
    {
        assign(init.begin(), init.end());
    }

    explicit bimap(size_t nreserve) { _data.reserve(nreserve); }

    iterator insert(value_type&& value)
    {
        auto [it_key1, okay_key1]
                = _first.try_emplace(value.first, _data.size());
        auto [it_key2, okay_key2]
                = _second.try_emplace(value.second, _data.size());

        if (not okay_key1) {
            if (okay_key2) { _second.erase(it_key2); }
            return _data.end();
        }
        if (not okay_key2) {
            if (okay_key1) { _first.erase(it_key1); }
            return _data.end();
        }

        _data.emplace_back(std::move(value));
        return _data.end() - 1;
    }

    iterator insert(value_type const& value)
    {
        return this->insert(value_type{value});
    }

    template <typename Iter>
    void assign(Iter begin, Iter end)
    {
        clear();
        reserve(std::distance(begin, end));

        while (begin != end) {}
    }

    template <typename FirstKey>
    auto find_first_key(FirstKey&& key) noexcept
    {
        auto iter = _first.find(std::forward<FirstKey>(key));
        if (iter == _first.end()) { return _data.end(); }

        return _data.begin() + iter->second;
    }

    template <typename SecondKey>
    auto find_second_key(SecondKey&& key) noexcept
    {
        return _second.equal_range(std::forward<SecondKey>(key));
    }
};
}  // namespace CPPHEADERS_NS_
