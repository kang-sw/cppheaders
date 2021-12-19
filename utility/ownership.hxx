#pragma once
#include <cassert>
#include <exception>
#include <type_traits>

#include "../__namespace__.h"

namespace CPPHEADERS_NS_ {
struct bad_ownership_access : std::exception
{
};

template <typename Ty_>
class ownership
{
   private:
    void _verify_access() const
    {
        if (not has_value())
            throw bad_ownership_access{};
    }

    template <typename... Args_>
    void _construct(Args_&&... args)
    {
        assert(_value == nullptr);
        _value = new (_storage) Ty_(std::forward<Args_>(args)...);
    }

   public:
    void reset() noexcept(std::is_nothrow_destructible_v<Ty_>)
    {
        if (has_value())
        {
            (*_value).~Ty_();
            _value = nullptr;
        }
    }

    template <typename... Args_>
    Ty_& emplace(Args_&&... args) noexcept(std::is_nothrow_constructible_v<Ty_, Args_...>)
    {
        reset();
        _construct(std::forward<Args_>(args)...);
        return value();
    }

    Ty_& operator*() { return _verify_access(), *_value; }

    Ty_ const& operator*() const { return _verify_access(), *_value; }

    Ty_* operator->() { return _verify_access(), _value; }

    Ty_ const* operator->() const { return _verify_access(), _value; }

    Ty_& value() { return _verify_access(), *_value; }

    Ty_ const& value_or(Ty_ const& alter) const noexcept
    {
        if (has_value())
            return *_value;
        else
            return alter;
    }

    Ty_* pointer() noexcept { return _value; }

    Ty_ const* pointer() const noexcept { return _value; }

    bool has_value() const noexcept { return _value; }

    operator bool() const noexcept { return has_value(); }

    ownership& operator=(ownership&& other) noexcept(std::is_nothrow_move_constructible_v<Ty_>)
    {
        if (not other)
        {
            reset();
        }
        else
        {
            if (has_value())
            {
                value() = std::move(other.value());
            }
            else
            {
                _construct(std::move(other.value()));
            }

            other.reset();
        }

        return *this;
    }

    ownership& operator=(Ty_ const& other) noexcept(
            std::is_nothrow_copy_constructible_v<Ty_>&& std::is_nothrow_copy_assignable_v<Ty_>)
    {
        if (has_value())
        {
            *_value = other;
        }
        else
        {
            _construct(other);
        }
    }

    ownership& operator=(Ty_&& other) noexcept(
            std::is_nothrow_move_constructible_v<Ty_>&& std::is_nothrow_move_assignable_v<Ty_>)
    {
        if (has_value())
        {
            *_value = std::move(other);
        }
        else
        {
            _construct(std::move(other));
        }

        return *this;
    }

    ownership(ownership&& other) noexcept(
            std::is_nothrow_move_constructible_v<Ty_>&& std::is_nothrow_destructible_v<Ty_>)
    {
        if (other)
        {
            _construct(std::move(other.value()));
            other.reset();
        }
    }

    ownership(Ty_&& other) noexcept(std::is_nothrow_move_constructible_v<Ty_>)
    {
        _construct(std::move(other));
    }

    ownership(Ty_ const& other) noexcept(std::is_nothrow_copy_constructible_v<Ty_>)
    {
        _construct(other);
    }

    ownership() noexcept = default;

    ~ownership() noexcept(std::is_nothrow_destructible_v<Ty_>)
    {
        reset();
    }

   private:
    Ty_* _value = nullptr;
    char _storage[sizeof(Ty_)];
};
}  // namespace CPPHEADERS_NS_