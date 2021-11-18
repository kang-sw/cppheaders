#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "hasher.hxx"
#include "spinlock.hxx"

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_ {
template <typename Mutex_>
struct lock_guard {
  enum { nothrow_lockable = std::is_nothrow_invocable_v<decltype(&Mutex_::lock), Mutex_*> };
  enum { nothrow_unlockable = std::is_nothrow_invocable_v<decltype(&Mutex_::unlock), Mutex_*> };

  lock_guard(Mutex_& mtx) noexcept(nothrow_lockable) : _ref(mtx) {
    _ref.lock();
  }

  void lock() noexcept(nothrow_lockable) {
    _ref.lock();
    _locked = true;
  }

  void unlock() noexcept(nothrow_unlockable) {
    _ref.unlock();
    _locked = false;
  }

  ~lock_guard() noexcept(nothrow_unlockable) {
    if (_locked)
      _ref.unlock();
  }

  bool _locked = true;
  Mutex_& _ref;
};

using delegate_key = key_base<class LABEL_delegate_key>;

enum class delegate_invoke_result {
  ok      = 0,
  expire  = 1,
  consume = 2,
};

inline delegate_invoke_result operator|(delegate_invoke_result a, delegate_invoke_result b) {
  return (delegate_invoke_result)(int(a) | int(b));
}

enum { DELEGATE_BITS = 61 };

enum class delegate_priority : uint64_t {
  last      = 0,
  very_low  = 1ull << DELEGATE_BITS,
  low       = 2ull << DELEGATE_BITS,
  middle    = 3ull << DELEGATE_BITS,
  high      = 4ull << DELEGATE_BITS,
  very_high = 5ull << DELEGATE_BITS,
  first     = ~uint64_t{},
};

template <typename Mutex_, typename... Args_>
class basic_delegate {
 public:
  using event_fn = std::function<delegate_invoke_result(Args_...)>;

  struct _entity_type {
    delegate_key id;
    event_fn function;
    uint64_t priority = 0;

    bool operator<(_entity_type const& rhs) const noexcept {
      // send pending-remove elements to back
      if (not id && rhs.id)
        return false;
      if (id && not rhs.id)
        return true;

      return priority < rhs.priority;
    }
  };

  static_assert(std::is_nothrow_move_assignable_v<_entity_type>);
  static_assert(std::is_nothrow_move_constructible_v<_entity_type>);

 public:
  using container  = std::vector<_entity_type>;
  using mutex_type = Mutex_;

 public:
  class handle {
    friend class basic_delegate;
    basic_delegate* owner_ = {};
    delegate_key key_      = {};

    handle(basic_delegate* o, delegate_key key) noexcept
            : owner_{o}, key_{key} {}

   public:
    auto& operator=(handle&& o) noexcept {
      owner_   = o.owner_;
      key_     = o.key_;
      o.owner_ = nullptr;
      o.key_   = {};
      return *this;
    }

    handle(handle&& other) noexcept { (*this) = std::move(other); }
    void expire() noexcept { owner_->remove(std::move(*this)); }
    bool valid() noexcept { return owner_ != nullptr; }
  };

 public:
  basic_delegate() noexcept = default;
  basic_delegate(basic_delegate&& rhs) noexcept { *this = std::move(rhs); }
  basic_delegate(basic_delegate const& rhs) noexcept { *this = rhs; }
  basic_delegate& operator=(basic_delegate&& rhs) noexcept {
    lock_guard a{_mtx}, b{rhs._mtx};
    _events = std::move(rhs._events);
    _mtx.unlock(), rhs._mtx.unlock();
  }
  basic_delegate& operator=(basic_delegate const& rhs) noexcept {
    lock_guard a{_mtx}, b{rhs._mtx};
    _events = rhs._events;
    _mtx.unlock(), rhs._mtx.unlock();
  }

  template <typename... FnArgs_>
  void invoke(FnArgs_&&... args) {
    lock_guard lock{_mtx};

    if (_dirty) {
      _dirty = false;
      std::sort(_events.begin(), _events.end());
    }

    for (auto it = _events.begin(); it != _events.end();) {
      if (not it->id) {
        _events.erase(it++);
      } else {
        lock.unlock();
        auto invoke_result = (int)it->function(std::forward<FnArgs_>(args)...);
        lock.lock();

        if (invoke_result & (int)delegate_invoke_result::expire)
          _events.erase(it++);
        else
          ++it;

        if (invoke_result & (int)delegate_invoke_result::consume)
          break;
      }
    }
  }

  template <typename Callable_>
  handle add(Callable_&& fn,
             delegate_priority priority = delegate_priority::last,
             uint64_t value             = 0) {
    lock_guard _{_mtx};
    auto* evt     = &_events.emplace_back();
    evt->id       = {++_hash_gen};
    evt->priority = (value & (1ull << DELEGATE_BITS) - 1) + (uint64_t)priority;

    _dirty |= evt->priority != 0;

    if constexpr (std::is_invocable_r_v<delegate_invoke_result, Callable_, Args_...>) {
      evt->function = (std::forward<Callable_>(fn));
    } else if constexpr (std::is_invocable_r_v<bool, Callable_, Args_...>) {
      evt->function = ([_fn = std::forward<Callable_>(fn)](auto&&... args) {
        return _fn(args...) ? delegate_invoke_result::ok
                            : delegate_invoke_result::expire;
      });
    } else if constexpr (std::is_invocable_v<Callable_>) {
      evt->function = ([_fn = std::forward<Callable_>(fn)](auto&&... args) {
        return _fn(), delegate_invoke_result::ok;
      });
    } else {
      evt->function = ([_fn = std::forward<Callable_>(fn)](auto&&... args) {
        return _fn(args...), delegate_invoke_result::ok;
      });
    }

    return handle{this, evt->id};
  }

  void priority(handle const& h, delegate_priority offset, uint64_t value = 0) noexcept {
    lock_guard _{_mtx};
    auto entity = _find(h);
    if (not entity)
      return;

    value &= (1ull << DELEGATE_BITS) - 1;
    value += (uint64_t)offset;
    entity->priority = value;
    _dirty           = true;
  }

  void remove(handle h) noexcept {
    lock_guard _{_mtx};
    auto entity = _find(h);
    if (not entity)
      return;

    entity->id = {};
    _dirty     = true;
  }

  bool empty() const noexcept {
    lock_guard _{_mtx};
    return _events.empty();
  }

  bool size() const noexcept {
    lock_guard _{_mtx};
    return _events.size();
  }

  template <typename Callable_>
  auto& operator+=(Callable_&& rhs) {
    add(std::forward<Callable_>(rhs));
    return *this;
  }

  auto& operator-=(handle&& r) noexcept {
    remove(std::move(r));
    return *this;
  }

 private:
  _entity_type* _find(handle const& h) {
    if (not h.key_)
      throw std::logic_error{"invalid handle!"};

    auto it = std::find_if(_events.begin(), _events.end(),
                           [&](_entity_type const& s) { return h.key_ == s.id; });
    return it == _events.end() ? nullptr : &*it;
  }

 private:
  container _events;
  uint64_t _hash_gen = 0;

  mutable Mutex_ _mtx;
  volatile bool _dirty = false;
};

struct null_mutex {
  bool try_lock() noexcept { return true; }
  void lock() noexcept {}
  void unlock() noexcept {}
};

template <typename... Args_>
using delegate = basic_delegate<spinlock, Args_...>;

template <typename... Args_>
using delegate_single_thread = basic_delegate<null_mutex, Args_...>;

}  // namespace CPPHEADERS_NS_
