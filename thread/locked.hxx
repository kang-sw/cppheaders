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

#include "../spinlock.hxx"

//
#include "../__namespace__"

namespace CPPHEADERS_NS_ {
template <typename Ty_, typename Mutex_ = perfkit::spinlock>
class locked
{
   public:
    struct locked_reference
    {
        Ty_* _ptr = nullptr;
        std::unique_lock<Mutex_> _lock;

       public:
        auto* operator->() { return _ptr; }
        auto* operator->() const { return _ptr; }
        auto& operator*() { return *_ptr; }
        auto& operator*() const { return *_ptr; }

        operator bool() const { return _lock; }
    };

    struct locked_const_reference
    {
        Ty_ const* _ptr = nullptr;
        std::unique_lock<Ty_> _lock;

       public:
        auto* operator->() { return _ptr; }
        auto* operator->() const { return _ptr; }
        auto& operator*() { return *_ptr; }
        auto& operator*() const { return *_ptr; }

        operator bool() const { return _lock; }
    };

   public:
    using value_type = Ty_;
    using lock_type  = Mutex_;

   public:
    locked_reference lock()
    {
        return {&_value, std::unique_lock{_mut}};
    }

    locked_const_reference lock() const
    {
        return {&_value, std::unique_lock{_mut}};
    }

    locked_reference try_lock()
    {
        return {&_value, std::unique_lock{_mut, std::try_to_lock}};
    }

    locked_const_reference try_lock() const
    {
        return {&_value, std::unique_lock{_mut, std::try_to_lock}};
    }

    template <typename Visitor_>
    void use(Visitor_&& visitor)
    {
        std::lock_guard _{_mut};
        visitor(_value);
    }

    template <typename Visitor_>
    void use(Visitor_&& visitor) const
    {
        std::lock_guard _{_mut};
        visitor(_value);
    }

    template <typename Visitor_>
    void try_use(Visitor_&& visitor)
    {
        if (std::lock_guard _{_mut, std::try_to_lock})
            visitor(_value);
    }

    template <typename Visitor_>
    void try_use(Visitor_&& visitor) const
    {
        if (std::lock_guard _{_mut, std::try_to_lock})
            visitor(_value);
    }

   private:
    Ty_ _value;
    mutable Mutex_ _mut;
};

}  // namespace CPPHEADERS_NS_