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

#pragma once
#include <memory>
#include <stdexcept>
#include <vector>

namespace cpph {

template <typename KeyTy_, typename MapTy_,
          typename Comparator_ = std::less<KeyTy_>,
          typename Alloc_ = std::allocator<std::pair<KeyTy_, MapTy_>>>
class sorted_vector
{
   public:
    using key_type = KeyTy_;
    using mapped_type = MapTy_;
    using value_type = std::pair<KeyTy_, MapTy_>;
    using allocator_type = Alloc_;
    using vector_type = std::vector<std::pair<KeyTy_, MapTy_>, Alloc_>;

    using iterator = typename vector_type::iterator;
    using const_iterator = typename vector_type::const_iterator;
    using reverse_iterator = typename vector_type::reverse_iterator;
    using const_reverse_iterator = typename vector_type::const_reverse_iterator;

    using comparator_type = Comparator_;

   private:
    vector_type _vector;

   private:
    template <typename Key_>
    const_iterator _lower_bound(Key_ const& key) const
    {
        return std::lower_bound(
                _vector.begin(), _vector.end(),
                key,
                [](value_type const& a, auto const& b) { return a.first < b; });
    }

    template <typename Key_>
    iterator _lower_bound(Key_ const& key)
    {
        return std::lower_bound(
                _vector.begin(), _vector.end(),
                key,
                [](value_type const& a, auto const& b) { return a.first < b; });
    }

    static bool _sort_fn(value_type const& a, value_type const& b) noexcept
    {
        return Comparator_{}(a.first, b.first);
    }

    template <class Iter1_, class Iter2_>
    static bool _less(Iter1_ const& a, Iter2_ const& b) noexcept
    {
        return Comparator_{}(a->first, b->first);
    }

   public:
    auto size() const noexcept { return _vector.size(); }
    auto empty() const noexcept { return _vector.empty(); }
    auto shrink_to_fit() { _vector.shrink_to_fit(); }
    auto reserve(size_t n) { _vector.reserve(n); }

    auto begin() noexcept { return _vector.begin(); }
    auto begin() const noexcept { return _vector.begin(); }
    auto end() noexcept { return _vector.end(); }
    auto end() const noexcept { return _vector.end(); }
    auto cbegin() const noexcept { return _vector.cbegin(); }
    auto cend() const noexcept { return _vector.cend(); }
    auto rbegin() const noexcept { return _vector.rbegin(); }
    auto rbegin() noexcept { return _vector.rbegin(); }
    auto rend() const noexcept { return _vector.rend(); }
    auto rend() noexcept { return _vector.rend(); }

    auto front() noexcept { return _vector.front(); }
    auto back() noexcept { return _vector.front(); }
    auto front() const noexcept { return _vector.front(); }
    auto back() const noexcept { return _vector.front(); }

    auto erase(const_iterator iter) { return _vector.erase(iter); }
    void clear() { _vector.clear(); }

    template <typename Key_>
    auto lower_bound(Key_ const& key) const
    {
        return _lower_bound(key);
    }

    template <typename Key_>
    auto lower_bound(Key_ const& key)
    {
        return _lower_bound(key);
    }

    template <typename Key_>
    auto find(Key_ const& key) const
    {
        auto it = _lower_bound(key);
        return it != _vector.end() && Comparator_{}(key, it->first) ? _vector.end() : it;
    }

    template <typename Key_>
    auto find(Key_ const& key)
    {
        auto it = _lower_bound(key);
        return it != _vector.end() && Comparator_{}(key, it->first) ? _vector.end() : it;
    }

    template <typename Key_>
    auto& at(Key_ const& key) const
    {
        auto it = _find(key);
        if (it == _vector.end()) { throw std::out_of_range{"key not exist"}; }

        return it->second;
    }

    template <typename Key_>
    auto& at(Key_ const& key)
    {
        auto it = _find(key);
        if (it == _vector.end()) { throw std::out_of_range{"key not exist"}; }

        return it->second;
    }

    template <typename Key_, typename... Args_>
    auto try_emplace(Key_&& key, Args_&&... args)
    {
        auto it = _lower_bound(key);
        if (it != _vector.end() && not Comparator_{}(key, it->first)) { return std::make_pair(it, false); }

        it = _vector.insert(
                it, value_type{
                            std::forward<Key_>(key),
                            mapped_type{std::forward<Args_>(args)...}});

        return std::make_pair(it, true);
    }

    template <typename Key_, typename... Args_>
    auto try_emplace_hint(size_t idx_hint, Key_&& key, Args_&&... args)
    {
        auto hint = _vector.begin() + idx_hint;

        bool wrong_hint = idx_hint >= _vector.size();
        if (wrong_hint)
            ;
        else if (_vector.empty())
            wrong_hint = (idx_hint != 0);
        else if (hint == _vector.end() && not(_vector.back().first < key))
            wrong_hint = true;
        else if (hint == _vector.begin() && not Comparator_(key, _vector.front().first))
            wrong_hint = true;
        else if (not Comparator_{}(hint[0].first, key))
            wrong_hint = true;
        else if (hint + 1 != _vector.end() && not Comparator_{}(key, (hint + 1)->first))
            wrong_hint = true;

        if (wrong_hint) {
            return try_emplace_hint(
                    std::forward<Key_>(key), std::forward<Args_>(args)...);
        } else {
            auto it = _vector.insert(
                    hint, value_type{
                                  std::forward<Key_>(key),
                                  mapped_type{std::forward<Args_>(args)...}});

            return std::make_pair(it, true);
        }
    }

    template <typename Iter_>
    void assign(Iter_ begin, Iter_ end)
    {
        _vector.assign(begin, end);
        std::sort(_vector.begin(), _vector.end(), _sort_fn);

        if (std::adjacent_find(_vector.begin(), _vector.end(), _sort_fn) != _vector.end())
            throw std::logic_error{"duplicated key found!"};
    }

    size_t erase(key_type key)
    {
        auto iter = find(key);

        if (iter == end()) {
            return 0;
        } else {
            _vector.erase(iter);
            return 1;
        }
    }
};
}  // namespace cpph