#pragma once
#include "assert.hxx"

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_ {
/**
 * Static sized vector class
 */
template <typename Ty_, size_t N_>
class static_vector {
 public:
  using value_type    = Ty_;
  using pointer       = value_type*;
  using const_pointer = value_type const*;
  using reference     = value_type&;

  using iterator       = pointer;
  using const_iterator = const_pointer;

 private:
  enum {
    _nt_dtor         = std::is_nothrow_destructible_v<Ty_>,
    _nt_move         = not std::is_move_assignable_v<Ty_> || std::is_nothrow_move_assignable_v<Ty_>,
    _nt_copy         = not std::is_copy_assignable_v<Ty_> || std::is_nothrow_move_assignable_v<Ty_>,
    _nt_ctor_default = not std::is_default_constructible_v<Ty_> || std::is_nothrow_default_constructible_v<Ty_>,
    _nt_ctor_move    = not std::is_move_constructible_v<Ty_> || std::is_nothrow_move_constructible_v<Ty_>,
    _nt_ctor_copy    = not std::is_copy_constructible_v<Ty_> || std::is_nothrow_copy_constructible_v<Ty_>,
  };

 private:
  void _verify_iterator(const_iterator iter) const {
    if (not(iter >= _ptr && iter < _ptr + N_))
      throw std::out_of_range{"invalid access"};
  }

  void _verify_iterator_range(const_iterator begin, const_iterator end) const {
    if (begin > end)
      throw std::logic_error{"invalid iterator order"};

    _verify_iterator(begin);
    _verify_iterator(end);
  }

  constexpr void _verify_idx(size_t idx) const {
    if (idx >= _size)
      throw std::out_of_range{"bad index"};
  }

  constexpr void _verify_space(size_t num_new_elems = 1) const {
    if (_size + num_new_elems > capacity())
      throw std::bad_alloc{};
  }

  template <typename... Args_>
  constexpr auto*
  _construct_at(iterator ptr, Args_&&... args) const
          noexcept(std::is_nothrow_constructible_v<Ty_, Args_...>) {
    return new (ptr) Ty_{std::forward<Args_>(args)...};
  }

  constexpr void _destruct_at(iterator ptr) const
          noexcept(std::is_nothrow_destructible_v<Ty_>) {
    (*ptr).~Ty_();
  }

  constexpr void
  _move_range(iterator begin, iterator end, iterator to_begin) noexcept(_nt_move&& _nt_dtor) {
    if (to_begin == begin)
      return;

    _verify_iterator_range(begin, end);
    auto to_end = to_begin + (end - begin);

    if (begin > to_begin) {
      while (--end, --to_end != to_begin - 1) {
        _construct_at(to_end, std::move(*end));
        _destruct_at(end);
      }
    } else {
      _verify_space(to_begin - begin);

      for (; begin != end; ++begin, ++to_begin) {
        _construct_at(to_begin, std::move(*begin));
        _destruct_at(begin);
      }
    }
  }

  template <bool Move_, typename It_>
  void _insert(iterator at, It_ begin, It_ end) noexcept(_nt_dtor&& _nt_ctor_move) {
    auto num_insert = std::distance(begin, end);
    _move_range(at, this->end(), begin + num_insert);
    _size += num_insert;
    for (; begin != end; ++begin)
      if constexpr (Move_)
        _construct_at(at++, std::move(*begin));
      else
        _construct_at(at++, *begin);
  }

 public:
  constexpr static_vector() noexcept = default;

  template <class It_>
  constexpr static_vector(It_ begin, It_ end) noexcept(std::is_nothrow_constructible_v<Ty_, decltype(*std::declval<It_>())>) {
    assign(begin, end);
  };

  constexpr static_vector(std::initializer_list<Ty_> list) noexcept(_nt_ctor_move)
          : static_vector(list.begin(), list.end()) {}

  constexpr static_vector(size_t n, Ty_ const& t = {}) noexcept(_nt_ctor_default) {
    resize(n, t);
  }

  auto& operator=(static_vector const& r) noexcept(_nt_ctor_copy&& _nt_move&& _nt_dtor) {
    assign(r.begin(), r.end());
  }

  auto& operator=(static_vector&& r) noexcept(_nt_ctor_move&& _nt_dtor) {
    erase(begin(), end());
    _insert<true>(begin(), r.begin(), r.end());
  }

  constexpr auto size() const noexcept { return _size; }
  constexpr auto data() const noexcept { return _ptr; }
  constexpr auto data() noexcept { return _ptr; }

  constexpr auto begin() noexcept { return _ptr; }
  constexpr auto begin() const noexcept { return _ptr; }
  constexpr auto end() noexcept { return _ptr + _size; }
  constexpr auto end() const noexcept { return _ptr + _size; }

  constexpr auto& front() const noexcept { return at(0); }
  constexpr auto& back() const noexcept { return at(_size - 1); }

  constexpr auto empty() const noexcept { return size() == 0; }
  constexpr size_t capacity() const noexcept { return N_; }

  template <typename... Args_>
  constexpr Ty_& emplace_back(Args_&&... r) {
    _verify_space();
    return *_construct_at(_ptr + _size++, std::forward<Args_>(r)...);
  }

  constexpr void push_back(Ty_ const& r)  //
          noexcept(std::is_nothrow_copy_assignable_v<Ty_>) {
    emplace_back(r);
  }

  constexpr void push_back(Ty_&& r)  //
          noexcept(std::is_nothrow_move_assignable_v<Ty_>) {
    emplace_back(std::move(r));
  }

  void pop_back() noexcept(_nt_dtor) {
    erase(end() - 1);
  }

  void clear() noexcept(_nt_dtor) {
    erase(begin(), end());
  }

  void resize(size_t new_size, Ty_ const& t = {}) noexcept(_nt_dtor&& _nt_ctor_default) {
    if (new_size < _size)
      erase(begin() + new_size, end());
    else if (_size < new_size) {
      _verify_space(new_size - _size);
      while (_size < new_size)
        _construct_at(_ptr + _size++, t);
    }
  }

  template <typename It_>
  constexpr void insert(iterator at, It_ begin, It_ end)  //
          noexcept(std::is_nothrow_constructible_v<Ty_, decltype(*std::declval<It_>())>&& _nt_move) {
    _insert<std::is_rvalue_reference_v<decltype(*std::declval<It_>())>>(
            at, begin, end);
  }

  constexpr void insert(iterator at, Ty_ const& v)  //
          noexcept(_nt_ctor_copy&& _nt_move) {
    _move_range(at, end(), at + 1);
    ++_size;
    _construct_at(at, v);
  }

  constexpr void insert(iterator at, Ty_&& v)  //
          noexcept(_nt_ctor_move&& _nt_move) {
    _move_range(at, end(), at + 1);
    ++_size;
    _construct_at(at, std::move(v));
  }

  template <typename It_>
  constexpr void assign(It_ begin, It_ end)  //
          noexcept(std::is_nothrow_constructible_v<Ty_, decltype(*std::declval<It_>())>) {
    erase(begin(), end());
    insert(this->begin(), begin, end);
  }

  iterator erase(iterator begin, iterator end)  //
          noexcept(_nt_dtor&& _nt_move) {
    _verify_iterator_range(begin, end);
    for (auto it = begin; it != end; ++it)
      _destruct_at(it);

    _move_range(end, this->end(), begin);
    _size -= end - begin;

    return begin;
  }

  iterator erase(iterator where)  //
          noexcept(_nt_dtor&& _nt_move) {
    return erase(where, where + 1);
  }

  template <typename RTy_>
  constexpr bool operator==(RTy_&& op) const noexcept {
    return std::equal(begin(), end(), std::begin(op), std::end(op));
  }

  template <typename RTy_>
  constexpr bool operator!=(RTy_&& op) const noexcept {
    return !(*this == std::forward<RTy_>(op));
  }

  template <typename RTy_>
  constexpr bool operator<(RTy_&& op) const noexcept {
    return std::lexicographical_compare(begin(), end(), std::begin(op), std::end(op));
  }

  constexpr auto& operator[](size_t idx) noexcept {
#ifndef NDEBUG
    _verify_idx(idx);
#endif
    return _ptr[idx];
  };

  constexpr auto& operator[](size_t idx) const noexcept {
#ifndef NDEBUG
    _verify_idx(idx);
#endif
    return _ptr[idx];
  };

  constexpr auto& at(size_t idx) { return _verify_idx(idx), _ptr[idx]; }
  constexpr auto& at(size_t idx) const { return _verify_idx(idx), _ptr[idx]; }

  ~static_vector() noexcept(_nt_dtor) {
    erase(begin(), begin() + _size);
    _size = 0;
  }

 private:
  std::array<std::byte, sizeof(Ty_) * N_> _buffer;
  Ty_* _ptr    = reinterpret_cast<Ty_*>(_buffer.data());
  size_t _size = 0;
};
}  // namespace CPPHEADERS_NS_