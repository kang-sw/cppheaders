#pragma once
#include <functional>
#include <list>
#include <memory>
#include <mutex>

#include "spinlock.hxx"

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_ {
enum class delegate_invoke_result {
  ok      = 0,
  expire  = 1,
  consume = 2,
};

inline delegate_invoke_result operator|(delegate_invoke_result a, delegate_invoke_result b) {
  return (delegate_invoke_result)(int(a) | int(b));
}

template <typename Mutex_, typename... Args_>
class basic_delegate {
 public:
  using event_fn           = std::function<delegate_invoke_result(Args_...)>;
  using event_pointer      = std::shared_ptr<event_fn>;
  using event_weak_pointer = std::weak_ptr<event_fn>;
  using container          = std::list<event_pointer>;
  using container_iterator = typename container::iterator;
  using mutex_type         = Mutex_;

 public:
  class handle {
    friend class basic_delegate;
    basic_delegate* owner   = {};
    container_iterator iter = {};
    event_weak_pointer ptr  = {};

    handle(basic_delegate* o, container_iterator i, event_weak_pointer p) noexcept
            : owner(o), iter(i), ptr(p) {}

   public:
    auto& operator=(handle&& o) noexcept {
      std::memcpy(this, &o, sizeof *this);
      std::memset(&o, 0, sizeof o);
    }

    handle(handle&& other) noexcept { (*this) = std::move(other); }
    void expire() noexcept { owner->remove(std::move(*this)); }
    bool valid() noexcept { return owner != nullptr; }
  };

 public:
  basic_delegate() noexcept = default;
  basic_delegate(basic_delegate&& rhs) noexcept { *this = std::move(rhs); }
  basic_delegate(basic_delegate const& rhs) noexcept { *this = rhs; }
  basic_delegate& operator=(basic_delegate&& rhs) noexcept {
    std::lock(_mtx, rhs._mtx);
    _events = std::move(rhs._events);
    _mtx.unlock(), rhs._mtx.unlock();
  }
  basic_delegate& operator=(basic_delegate const& rhs) noexcept {
    std::lock(_mtx, rhs._mtx);
    _events = rhs._events;
    _mtx.unlock(), rhs._mtx.unlock();
  }

  template <typename... FnArgs_>
  void invoke(FnArgs_&&... args) {
    std::unique_lock lock{_mtx};
    for (auto it = _events.begin(); it != _events.end();) {
      auto ptr = *it;
      if (not ptr) {
        _events.erase(it++);
      } else {
        lock.unlock();
        auto invoke_result = (int)(*ptr)(std::forward<FnArgs_>(args)...);
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
  handle add(Callable_&& fn) noexcept {
    std::lock_guard _{_mtx};
    event_pointer ptr;

    if constexpr (std::is_invocable_r_v<delegate_invoke_result, Callable_, Args_...>) {
      ptr = std::make_shared<event_fn>(std::forward<Callable_>(fn));
    } else if constexpr (std::is_invocable_r_v<bool, Callable_, Args_...>) {
      ptr = std::make_shared<event_fn>(
              [_fn = std::forward<Callable_>(fn)](auto&&... args) {
                return _fn(args...) ? delegate_invoke_result::ok
                                    : delegate_invoke_result::expire;
              });
    } else if constexpr (std::is_invocable_v<Callable_>) {
      ptr = std::make_shared<event_fn>(
              [_fn = std::forward<Callable_>(fn)](auto&&... args) {
                return _fn(), delegate_invoke_result::ok;
              });
    } else {
      ptr = std::make_shared<event_fn>(
              [_fn = std::forward<Callable_>(fn)](auto&&... args) {
                return _fn(args...), delegate_invoke_result::ok;
              });
    }

    _events.emplace_back(std::move(ptr));
    auto iter = --_events.end();
    return handle{this, iter, *iter};
  }

  void remove(handle it) {
    std::lock_guard _{_mtx};
    if (not it.ptr.expired())
      it.iter->reset();
  }

  bool empty() const noexcept {
    std::lock_guard _{_mtx};
    return _events.empty();
  }

  bool size() const noexcept {
    std::lock_guard _{_mtx};
    return _events.size();
  }

 private:
  container _events;
  mutable Mutex_ _mtx;
};

struct null_mutex {
  bool try_lock() noexcept { return true; }
  void lock() noexcept {}
  void unlock() noexcept {}
};

template <typename... Args_>
using delegate = basic_delegate<spinlock, Args_...>;

template <typename... Args_>
using delegate_unsafe = basic_delegate<null_mutex, Args_...>;

}  // namespace CPPHEADERS_NS_
