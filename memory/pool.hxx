#pragma once
#include <list>
#include <vector>

#include "../spinlock.hxx"

//
#include "../__namespace__.h"

namespace CPPHEADERS_NS_
{
template <typename Ty_, typename Mutex_ = null_mutex>
class basic_resource_pool
{
   public:
    using buffer_type     = std::list<Ty_>;
    using buffer_iterator = typename buffer_type::iterator;

   public:
    struct handle_type
    {
       public:
        handle_type() noexcept = default;

        handle_type(handle_type&& r) noexcept
        {
            _assign(std::move(r));
        }

        handle_type& operator=(handle_type&& r) noexcept
        {
            ~handle_type();
            _assign(std::move(r));
            return *this;
        }

        ~handle_type() noexcept
        {
            valid() && (checkin(), 0);
        }

        void checkin()
        {
            assert(valid());
            _owner->checkin(std::move(*this));
        }

        bool valid() const noexcept
        {
            return _owner;
        }

        Ty_* operator->() noexcept
        {
            return &*_ref;
        }

        Ty_ const* operator->() const noexcept
        {
            return &*_ref;
        }

        Ty_& operator*() noexcept
        {
            return *_ref;
        }

        Ty_ const& operator*() const noexcept
        {
            return *_ref;
        }

       private:
        void _assign(handle_type&& other)
        {
            _owner       = other._owner;
            _ref         = other._ref;
            other._owner = nullptr;
            other._ref   = {};
        }

       private:
        friend class basic_resource_pool;

        basic_resource_pool* _owner = nullptr;
        typename std::list<Ty_>::iterator _ref{};
    };

   public:
    handle_type checkout()
    {
        lock_guard _{_mut};
        handle_type r;

        if (not _free.empty())
        {
            r._ref = _free.back();
            _free.pop_back();
            return r;
        }

        _pool.emplace_back();
        r._ref = --_pool.end();
        return r;
    }

    void checkin(handle_type h)
    {
        if (not h.valid())
            throw std::invalid_argument{"invalid handle"};

        if (h._owner != this)
            throw std::invalid_argument{"invalid owner reference"};

        lock_guard _{_mut};
        _free.push_back(h._ref);
    }

    void shrink()
    {
        lock_guard _{_mut};

        for (auto it : _free)
            _pool.erase(it);
    }

   private:
    Mutex_ _mut;
    std::list<Ty_> _pool;
    std::vector<buffer_iterator> _free;
};

template <typename Ty_>
using pool = basic_resource_pool<Ty_, perfkit::spinlock>;

template <typename Ty_>
using pool_ptr = typename pool<Ty_>::handle_type;

}  // namespace CPPHEADERS_NS_
