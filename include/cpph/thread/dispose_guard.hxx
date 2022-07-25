#pragma once
#include "threading.hxx"

namespace cpph {
class atomic_dispose_guard
{
    atomic_int _lc = 0;

   public:
    ~atomic_dispose_guard() noexcept
    {
        assert(relaxed(_lc) >= 0);

        for (;;) {
            while (relaxed(_lc) > 0) { _detail::thread_yield(); }

            int rv = 0;
            if (_lc.compare_exchange_weak(rv, std::numeric_limits<int>::min() / 2)) {
                return;
            }

            assert(rv >= 0);
        }
    }

    bool try_add_ref() noexcept
    {
        // Fast access ...
        auto rv = relaxed(_lc);
        if (rv < 0) { return false; }

        for (;;) {
            if (_lc.compare_exchange_weak(rv, rv + 1, std::memory_order_relaxed)) {
                return true;
            } else if (rv < 0) {
                // Lock disposed during acquiring shared lock ...
                return false;
            }
        }
    }

    bool dec_ref() noexcept
    {
        return 0 < _lc.fetch_sub(1, std::memory_order_relaxed);
    }
};
}  // namespace cpph
