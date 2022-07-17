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
    node* _first = nullptr;

   public:
    template <typename... Args>
    T& _emplace_with(void* buffer, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        _first = new (buffer) node{_first, {std::forward<Args>(args)...}};
        return _first->data;
    }

    auto begin() noexcept { return iterator{_first}; }
    auto begin() const noexcept { return const_iterator{_first}; }
    auto cbegin() const noexcept { return this->begin(); }
    auto end() noexcept { return iterator{nullptr}; }
    auto end() const noexcept { return const_iterator{nullptr}; }
    auto cend() const noexcept { return this->end(); }

   public:
    ~alloca_fwd_list() noexcept(std::is_nothrow_destructible_v<T>)
    {
        for (; _first; _first = _first->next) {
            _first->data.~T();
        }
    }
};
}  // namespace cpph::_detail
