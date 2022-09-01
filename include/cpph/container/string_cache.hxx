#pragma once
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
        string_cache const* owner_;
        node const* p_;

       public:
        using value_type = string_view;
        using pointer = string_view;
        using reference = string_view;
        using iterator_category = std::forward_iterator_tag;

       public:
        explicit const_iterator(string_cache const* owner, node* p) noexcept : owner_(owner), p_(p) {}
        bool operator==(const_iterator other) const noexcept { return p_ == other.p_; }
        bool operator!=(const_iterator other) const noexcept { return not(p_ == other.p_); }

        auto operator*() const noexcept
        {
            return string_view{owner_->payload_}.substr(p_->pos + sizeof(node), p_->str_len);
        }
        const_iterator& operator++() noexcept
        {
            p_ = (node const*)((char const*)p_ + sizeof(node) + align_ceil_(p_->str_len + 1));
            assert(([this, distance = (char const*)p_ - owner_->payload_.data()] {
                return distance >= 0 && distance < owner_->payload_.size();
            }()));

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

    void push_back(string_view s)
    {
        size_t pos = payload_.size();
        size_t len = s.size();

        payload_.reserve(sizeof(node) + align_ceil_(s.size() + 1));
        payload_.append((char*)&pos, sizeof pos);
        payload_.append((char*)&len, sizeof len);
        payload_.append(s);
        payload_.append(s.size() & (sizeof(size_t) - 1), '\0');  // Align to 8

        assert((payload_.size() & sizeof(size_t)) == 0);
    }

    void clear() noexcept
    {
        payload_.clear();
    }

    auto begin() const noexcept { return const_iterator{this, (node*)payload_.data()}; }
    auto end() const noexcept { return const_iterator{this, (node*)(payload_.data() + payload_.size())}; }
    auto cbegin() const noexcept { return begin(); }
    auto cend() const noexcept { return end(); }

   private:
    static size_t align_ceil_(size_t len) noexcept { return (len + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1); }
};
}  // namespace cpph
