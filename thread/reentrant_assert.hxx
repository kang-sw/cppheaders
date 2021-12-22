#pragma once
#include <mutex>

#include "../assert.hxx"
#include "../spinlock.hxx"

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_ {

class reentrant_assert
{
   public:
    auto operator()() const noexcept
    {
        std::unique_lock lc{_lock, std::try_to_lock};
        assert_(lc);
        return lc;
    }

   private:
    mutable perfkit::spinlock _lock;
};

}  // namespace CPPHEADERS_NS_