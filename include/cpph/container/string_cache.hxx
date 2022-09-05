#pragma once
#include <algorithm>
#include <cassert>
#include <cpph/std/string>
#include <cpph/std/string_view>
#include <cpph/std/tuple>

namespace cpph {
/**
 * Contains list of strings.
 */
class string_cache
{
    struct node {
        size_t pos;
        size_t str_len;
    };

   public:
    class const_iterator
    {
        string_cache const* s_;
        size_t pos_;

       public:
        using value_type = string_view;
        using pointer = string_view;
        using reference = string_view;
        using iterator_category = std::forward_iterator_tag;

       public:
        explicit const_iterator(string_cache const* owner, size_t pos) noexcept : s_(owner), pos_(pos) {}
        bool operator==(const_iterator other) const noexcept { return pos_ == other.pos_; }
        bool operator!=(const_iterator other) const noexcept { return not(pos_ == other.pos_); }

        auto operator*() const noexcept
        {
            auto n = s_->node_at_(pos_);
            assert(pos_ + sizeof(node) < s_->payload_.size());
            return string_view{s_->payload_.data() + pos_ + sizeof(node), n->str_len};
        }
        const_iterator& operator++() noexcept
        {
            auto n = s_->node_at_(pos_);
            pos_ += sizeof(node) + align_ceil_(n->str_len + 1);
            return *this;
        }
        const_iterator operator++(int) noexcept
        {
            auto prev = *this;
            return ++*this, prev;
        }
    };

   private:
    string payload_;

   public:
    string_cache() noexcept = default;
    string_cache(string_cache&&) noexcept = default;
    string_cache(string_cache const&) noexcept = default;
    string_cache& operator=(string_cache&&) noexcept = default;
    string_cache& operator=(string_cache const&) noexcept = default;

    void reserve(size_t num_chars, size_t num_strings = 0)
    {
        payload_.reserve(num_chars + (sizeof(node) + sizeof(size_t)) * num_strings);
    }

    template <class... Str>
    auto push_back(Str&&... strings)
    {
        assert((payload_.size() & (sizeof(size_t) - 1)) == 0);

        string_view views[] = {strings...};
        size_t pos = payload_.size();
        size_t str_len = 0;
        for (auto& s : views) { str_len += s.size(); }

        auto end_pos = payload_.size() + sizeof(node) + align_ceil_(str_len + 1);
        payload_.reserve(end_pos);
        payload_.append((char*)&pos, sizeof pos);
        payload_.append((char*)&str_len, sizeof str_len);

        for (auto& s : views) { payload_.append(s); }
        payload_.push_back('\0');

        // Fill with zeros until size align to size_t
        payload_.append(end_pos - payload_.size(), '\0');

        assert((payload_.size() & (sizeof(size_t) - 1)) == 0);
        return const_iterator(this, pos);
    }

    void clear() noexcept
    {
        payload_.clear();
    }

    auto begin() const noexcept { return const_iterator{this, 0}; }
    auto end() const noexcept { return const_iterator{this, payload_.size()}; }
    auto cbegin() const noexcept { return begin(); }
    auto cend() const noexcept { return end(); }

   private:
    static size_t align_ceil_(size_t len) noexcept { return (len + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1); }
    auto node_at_(size_t pos) const noexcept -> node const* { return (node const*)(payload_.data() + pos); }
};
}  // namespace cpph
