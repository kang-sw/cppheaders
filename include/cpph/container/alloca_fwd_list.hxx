#pragma once
#include <cpph/utility/templates.hxx>

namespace cpph::_detail {
template <class T>
struct alloca_fwd_list {
    struct node {
        node* next;
        T data;
    };

    enum { node_size = sizeof(node) };

    template <bool IsConst>
    struct basic_iterator {
        using node_type = conditional_t<IsConst, node const, node>;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = conditional_t<IsConst, T const, T>;
        using pointer = value_type*;
        using reference = value_type&;

        node_type* _cursor;

        basic_iterator& operator++() noexcept { return _cursor = _cursor->next, *this; }
        basic_iterator operator++(int) noexcept { return basic_iterator{exchange(_cursor, _cursor->next)}; }
        bool operator==(basic_iterator other) const noexcept { return _cursor == other._cursor; }
        bool operator!=(basic_iterator other) const noexcept { return _cursor != other._cursor; }
        auto& operator*() const noexcept { return _cursor->data; }
        auto* operator->() const noexcept { return &_cursor->data; }
    };

    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator = basic_iterator<false>;
    using const_iterator = basic_iterator<true>;

   public:
    node* first_ = nullptr;
    size_t nelem_ = 0;

   public:
    template <typename... Args>
    T& _emplace_with(void* buffer, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        first_ = new (buffer) node{first_, {std::forward<Args>(args)...}};
        ++nelem_;
        return first_->data;
    }

    auto begin() noexcept { return iterator{first_}; }
    auto begin() const noexcept { return const_iterator{first_}; }
    auto cbegin() const noexcept { return this->begin(); }
    auto end() noexcept { return iterator{nullptr}; }
    auto end() const noexcept { return const_iterator{nullptr}; }
    auto cend() const noexcept { return this->end(); }

    auto empty() const noexcept { return first_ == nullptr; }
    auto size() const noexcept { return nelem_; }

   public:
    ~alloca_fwd_list() noexcept(std::is_nothrow_destructible_v<T>)
    {
        for (; first_; first_ = first_->next) {
            first_->data.~T();
        }
    }
};
}  // namespace cpph::_detail
