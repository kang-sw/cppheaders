// MIT License
//
// Copyright (c) 2021-2022. Seungwoo Kang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// project home: https://github.com/perfkitpp

#pragma once
#include <mutex>

#include "spinlock.hxx"

//
namespace cpph {
template <typename Ty_, typename Mutex_ = spinlock>
class locked
{
   private:
    Ty_ _value;
    mutable Mutex_ _mut;

   public:
    class locked_reference
    {
        Ty_* _ptr = nullptr;
        std::unique_lock<Mutex_> _lock;

       public:
        locked_reference(Ty_* ptr, std::unique_lock<Mutex_> lock) noexcept
                : _ptr(ptr), _lock(std::move(lock)) {}

       public:
        auto* operator->() { return _ptr; }
        auto* operator->() const { return _ptr; }
        auto& operator*() { return *_ptr; }
        auto& operator*() const { return *_ptr; }

        //
        operator bool() const { return _lock; }
    };

    class locked_const_reference
    {
        Ty_ const* _ptr = nullptr;
        std::unique_lock<Mutex_> _lock;

       public:
        locked_const_reference(Ty_ const* ptr, std::unique_lock<Mutex_> lock) noexcept
                : _ptr(ptr), _lock(std::move(lock)) {}

       public:
        auto* operator->() { return _ptr; }
        auto* operator->() const { return _ptr; }
        auto& operator*() { return *_ptr; }
        auto& operator*() const { return *_ptr; }

        //
        operator bool() const { return _lock; }
    };

   public:
    using value_type = Ty_;
    using lock_type = Mutex_;

   public:
    template <typename... Args>
    explicit locked(Args&&... args) noexcept(std::is_nothrow_constructible_v<Ty_, Args...>)
            : _value(std::forward<Args>(args)...)
    {
    }

   public:
    auto lock()
    {
        return locked_reference{&_value, std::unique_lock{_mut}};
    }

    auto lock() const
    {
        return locked_const_reference{&_value, std::unique_lock{_mut}};
    }

    auto try_lock()
    {
        return locked_reference{&_value, std::unique_lock{_mut, std::try_to_lock}};
    }

    auto try_lock() const
    {
        return locked_const_reference{&_value, std::unique_lock{_mut, std::try_to_lock}};
    }

    auto unsafe_access() const
    {
        return &_value;
    }

    auto unsafe_access()
    {
        return &_value;
    }

    template <typename Visitor_>
    [[deprecated]] void use(Visitor_&& visitor)
    {
        std::lock_guard _{_mut};
        visitor(_value);
    }

    template <typename Visitor_>
    [[deprecated]] void use(Visitor_&& visitor) const
    {
        std::lock_guard _{_mut};
        visitor(_value);
    }

    template <typename Visitor_>
    void access(Visitor_&& visitor)
    {
        std::lock_guard _{_mut};
        visitor(_value);
    }
    template <typename Visitor_>
    void access(Visitor_&& visitor) const
    {
        std::lock_guard _{_mut};
        visitor(_value);
    }

    template <typename Visitor_>
    void try_access(Visitor_&& visitor)
    {
        if (std::lock_guard _{_mut, std::try_to_lock})
            visitor(_value);
    }

    template <typename Visitor_>
    void try_access(Visitor_&& visitor) const
    {
        if (std::lock_guard _{_mut, std::try_to_lock})
            visitor(_value);
    }

    template <typename Visitor_>
    [[deprecated]] void try_use(Visitor_&& visitor)
    {
        if (std::lock_guard _{_mut, std::try_to_lock})
            visitor(_value);
    }

    template <typename Visitor_>
    [[deprecated]] void try_use(Visitor_&& visitor) const
    {
        if (std::lock_guard _{_mut, std::try_to_lock})
            visitor(_value);
    }
};

}  // namespace cpph