#pragma once
#include <mutex>

#include "../spinlock.hxx"

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_
{
template <typename Ty_, typename Mutex_ = perfkit::spinlock>
class locked
{
   public:
    struct locked_reference
    {
        Ty_* _ptr = nullptr;
        std::unique_lock<Ty_> _lock;

       public:
        auto* operator->() { return _ptr; }
        auto* operator->() const { return _ptr; }
        auto& operator*() { return *_ptr; }
        auto& operator*() const { return *_ptr; }
    };

   public:
    locked_reference lock()
    {
        return {&_value, std::unique_lock{_mut}};
    }

    template <typename Visitor_>
    void use(Visitor_&& visitor)
    {
        std::lock_guard _{_mut};
        visitor(_value);
    }

   private:
    Ty_ _value;
    Mutex_ _mut;
};

}  // namespace CPPHEADERS_NS_