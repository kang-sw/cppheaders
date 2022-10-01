#pragma once
#include "cpph/thread/threading.hxx"

namespace cpph {

template <class Signature, class Mutex>
class event_registry;

template <class Mutex, typename... Args>
class event_registry<void(Args...), Mutex>
{
   public:
    template <class Callable, class... BindArgs>
    void push(Callable&& callable, BindArgs&&... args)
    {
    }

   private:
};

}  // namespace cpph