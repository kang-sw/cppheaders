#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <memory>

namespace KANGSW_TEMPLATE_NAMESPACE {
/**
 * 스레드에 매우 안전하지 않은 클래스입니다.
 * 별도의 스레드와 사용 시 반드시 락 필요
 */
template <typename Ty_>
class circular_queue {
  using chunk_t = std::array<int8_t, sizeof(Ty_)>;

 public:
  using value_type = Ty_;

  constexpr static bool is_safe_ctor = std::is_nothrow_constructible_v<Ty_>;
  constexpr static bool is_safe_dtor = std::is_nothrow_destructible_v<Ty_>;

 public:
  template <bool Constant_ = true, bool Reverse_ = false>
  class iterator {
   public:
    using owner_type        = std::conditional_t<Constant_, circular_queue const, circular_queue>;
    using value_type        = Ty_;
    using value_type_actual = std::conditional_t<Constant_, Ty_ const, Ty_>;
    using pointer           = value_type_actual const*;
    using reference         = value_type_actual&;
    using difference_type   = ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

   public:
    iterator() noexcept                = default;
    iterator(const iterator&) noexcept = default;
    iterator& operator=(const iterator&) noexcept = default;

    reference operator*() const {
      if constexpr (Reverse_) {
        return _owner->_r_at(_head);
      } else {
        return _owner->_at(_head);
      }
    }

    pointer operator->() const { return &*(*this); }

    bool operator==(iterator const& op) const noexcept { return _head == op._head; }
    bool operator!=(iterator const& op) const noexcept { return _head != op._head; }
    bool operator<(iterator const& op) const noexcept { return _idx() < op._idx(); }
    bool operator<=(iterator const& op) const noexcept { return !(op < *this); }

    auto& operator[](difference_type idx) const noexcept { return *(*this + idx); }

    auto& operator++() noexcept { return _head = _owner->_next(_head), *this; }
    auto& operator--() noexcept { return _head = _owner->_prev(_head), *this; }

    auto operator++(int) noexcept {
      auto c = *this;
      return ++*this, c;
    }
    auto operator--(int) noexcept {
      auto c = *this;
      return --*this, c;
    }

    auto& operator-=(ptrdiff_t i) noexcept { return _head = _owner->_jmp(_head, -i), *this; }
    auto& operator+=(ptrdiff_t i) noexcept { return _head = _owner->_jmp(_head, i), *this; }

    auto operator-(iterator const& op) const noexcept { return static_cast<ptrdiff_t>(_idx() - op._idx()); }

    friend auto operator+(iterator it, ptrdiff_t i) { return it += i; }
    friend auto operator+(ptrdiff_t i, iterator it) { return it += i; }
    friend auto operator-(iterator it, ptrdiff_t i) { return it -= i; }
    friend auto operator-(ptrdiff_t i, iterator it) { return it -= i; }

   private:
    size_t _idx() const noexcept { return _owner->_idx_linear(_head); }

   private:
    iterator(owner_type* o, size_t h)
            : _owner(o), _head(h) {}
    friend class circular_queue;

    owner_type* _owner;
    size_t _head;
  };

 public:
  circular_queue(size_t capacity) noexcept
          : _capacity(capacity + 1), _data(capacity ? std::make_unique<chunk_t[]>(_capacity) : nullptr) {}

  circular_queue(const circular_queue& op) noexcept(is_safe_ctor) { *this = op; }

  circular_queue(circular_queue&& op) noexcept {
    *this = std::move(op);
  }

  circular_queue& operator=(circular_queue&& op) noexcept {
    std::swap(_head, op._head);
    std::swap(_tail, op._tail);
    std::swap(_data, op._data);
    std::swap(_capacity, op._capacity);
    return *this;
  }

  circular_queue& operator=(const circular_queue& op) noexcept(is_safe_ctor) {
    clear();
    _head     = {};
    _tail     = {};
    _capacity = op._capacity;
    _data     = std::make_unique<chunk_t[]>(_capacity);

    std::copy(op.begin(), op.end(), begin());
    return *this;
  }

  void reserve_shrink(size_t new_cap) noexcept(is_safe_ctor) {
    if (new_cap == capacity()) { return; }
    if (new_cap == 0) { clear(), _data.reset(), _capacity = 1; }
    auto n_copy = std::min(size(), new_cap);

    // move available objects
    circular_queue next{new_cap};
    std::move(begin(), begin() + n_copy, std::back_inserter(next));

    // destroies unmoved objects
    _tail = _jmp(_tail, n_copy);
    clear();

    *this = std::move(next);
  }

  template <typename RTy_>
  void push(RTy_&& s) noexcept(is_safe_ctor) {
    new (_data[_reserve()].data()) Ty_(std::forward<RTy_>(s));
  }

  template <typename... Args_>
  void emplace(Args_&&... args) noexcept(is_safe_ctor) {
    new (_data[_reserve()].data()) Ty_(std::forward<Args_>(args)...);
  }

  template <typename RTy_>
  void rotate(RTy_&& s) noexcept(is_safe_ctor) {
    is_full() && (pop(), 0) || (this->push(std::forward<RTy_>(s)), 1);
  }

  template <typename... Args_>
  void emplace_rotate(Args_&&... args) noexcept(is_safe_ctor) {
    is_full() && (pop(), 0) || (this->emplace(std::forward<Args_>(args)...), 1);
  }

  template <typename RTy_>
  void push_back(RTy_&& s) noexcept(is_safe_ctor) {
    this->rotate(std::forward<RTy_>(s));
  }

  template <typename... Args_>
  void emplace_back(Args_&&... args) noexcept(is_safe_ctor) {
    this->emplace_rotate(std::forward<Args_>(args)...);
  }

  void pop() noexcept(is_safe_dtor) {
    _pop();
  }

  void pop(Ty_& dst) noexcept(is_safe_ctor&& is_safe_dtor) {
    (dst = std::move(front())), _pop();
  }

  template <typename... Args_>
  void enqueue(Args_&&... args) noexcept(is_safe_ctor) {
    this->emplace_rotate(std::forward<Args_>(args)...);
  }

  Ty_ dequeue() noexcept(is_safe_ctor&& is_safe_dtor) {
    Ty_ r = std::move(front());
    _pop();
    return r;
  }

  template <typename OutIt_>
  void dequeue_n(size_t n, OutIt_ oit) noexcept(is_safe_ctor&& is_safe_dtor) {
    if (n > size()) {
      throw std::out_of_range{
              to_string(n)
                      .append(" is larger than queue size ")
                      .append(to_string(size()))};
    }

    if constexpr (std::is_trivially_destructible_v<Ty_>) {
      std::copy(begin(), begin() + n, oit);
      _tail = _jmp(_tail, n);
    } else {
      while (n--) {
        oit = std::move(front());
        pop();
      }
    }
  }

  size_t size() const noexcept {
    return _head >= _tail ? _head - _tail : _head + _cap() - _tail;
  }

  auto cbegin() const noexcept { return iterator<true>(this, _tail); }
  auto cend() const noexcept { return iterator<true>(this, _head); }
  auto begin() const noexcept { return cbegin(); }
  auto end() const noexcept { return cend(); }
  auto begin() noexcept { return iterator<false>(this, _tail); }
  auto end() noexcept { return iterator<false>(this, _head); }

  auto crbegin() const noexcept { return iterator<true, true>(this, _tail); }
  auto crend() const noexcept { return iterator<true, true>(this, _head); }
  auto rbegin() const noexcept { return cbegin(); }
  auto rend() const noexcept { return cend(); }
  auto rbegin() noexcept { return iterator<false, true>(this, _tail); }
  auto rend() noexcept { return iterator<false, true>(this, _head); }

  constexpr size_t capacity() const noexcept { return _capacity - 1; }
  bool empty() const noexcept { return _head == _tail; }

  Ty_& front() noexcept { return _front(); }
  Ty_ const& front() const noexcept { return _front(); }

  Ty_& back() noexcept { return _back(); }
  Ty_ const& back() const noexcept { return _back(); }

  bool is_full() const noexcept { return _next(_head) == _tail; }

  template <class Fn_>
  void for_each(Fn_&& fn) {
    for (size_t it = _tail; it != _head; it = _next(it)) { fn(_at(it)); }
  }

  template <class Fn_>
  void for_each(Fn_&& fn) const {
    for (size_t it = _tail; it != _head; it = _next(it)) { fn(static_cast<const Ty_&>(_at(it))); }
  }

  void clear() noexcept(is_safe_dtor) {
    if constexpr (std::is_trivially_destructible_v<Ty_>) {
      _tail = _head;
    } else {
      while (!empty()) { pop(); }
    }
  }

  template <typename Fn_>
  void flat(Fn_&& fn) {
    if (_tail < _head) {
      fn(&_at(_tail), &_at(_head));
    } else if (_head < _tail) {
      fn(&_at(_tail), &_at(_capacity));
      fn(&_at(0), &_at(_head));
    }
  }

  ~circular_queue() noexcept(is_safe_dtor) { clear(); }

 private:
  size_t _cap() const noexcept { return _capacity; }

  size_t _reserve() {
    if (is_full()) throw std::bad_array_new_length();
    auto retval = _head;
    _head       = _next(_head);
    return retval;
  }

  size_t _next(size_t current) const noexcept {
    return ++current == _cap() ? 0 : current;
  }

  size_t _prev(size_t current) const noexcept {
    return --current == ~size_t{} ? _cap() - 1 : current;
  }

  size_t _jmp(size_t at, ptrdiff_t jmp) const noexcept {
    if (jmp >= 0) { return (at + jmp) % _cap(); }
    return at += jmp, at + _cap() * ((ptrdiff_t)at < 0);
  }

  size_t _idx_linear(size_t i) const noexcept {
    if (_head >= _tail) { return i; }
    return i - _tail * (i >= _tail) + (_cap() - _tail) * (i < _tail);
  }

  void _pop() {
    assert(!empty());
    reinterpret_cast<Ty_&>(_data[_tail]).~Ty_();
    _tail = _next(_tail);
  }

  Ty_& _front() const {
    assert(!empty());
    return reinterpret_cast<Ty_&>(const_cast<chunk_t&>(_data[_tail]));
  }

  Ty_& _at(size_t i) const {
    assert(!empty());
    return reinterpret_cast<Ty_&>(const_cast<chunk_t&>(_data[i]));
  }

  Ty_& _r_at(size_t i) const {
    assert(!empty());
    return _at(_jmp(_head, -_idx_linear(i + 1)));
  }

  Ty_& _back() const {
    assert(!empty());
    return _at(_prev(_head));
  }

 private:
  size_t _capacity;
  std::unique_ptr<chunk_t[]> _data;
  size_t _head = {};
  size_t _tail = {};
};
}  // namespace KANGSW_TEMPLATE_NAMESPACE
