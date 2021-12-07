#pragma once
#include <array>
#include <cstddef>
#include <functional>

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_ {

constexpr int _function_size = sizeof(std::function<void()>) + 16;

template <typename Signature_>
class function;

template <typename Ret_, typename... Args_>
class function<Ret_(Args_...)>
{
   public:
    using return_type   = Ret_;
    using function_type = std::function<Ret_(Args_...)>;

   private:
    struct _callable_t
    {
        virtual ~_callable_t()                 = default;
        virtual Ret_ operator()(Args_... args) = 0;
        virtual void move(void* to)            = 0;
    };

   private:
    template <typename Callable_>
    void _assign_function(Callable_&& fn)
    {
        using callable_type = std::remove_cv_t<std::remove_reference_t<Callable_>>;

        struct _callable_impl_t : _callable_t
        {
            Ret_ operator()(Args_... args) override
            {
                return std::invoke(_body, std::forward<Args_>(args)...);
            }

            explicit _callable_impl_t(Callable_&& fwd)
                    : _body(std::forward<Callable_>(fwd))
            {
            }

            void move(void* raw) override
            {
                new (raw) _callable_impl_t(std::move(*this));
            }

           private:
            callable_type _body;
        };

        if constexpr (sizeof(_callable_impl_t) <= sizeof _sbo_buf)
        {  // allocate on heap
            _callable = new (_sbob()) _callable_impl_t{std::forward<Callable_>(fn)};
        }
        else
        {  // apply SBO
            _callable = new _callable_impl_t{std::forward<Callable_>(fn)};
        }
    }

    void _move_from(function&& rhs) noexcept
    {
        if (not rhs.is_sbo())
        {
            _callable     = rhs._callable;
            rhs._callable = nullptr;
        }
        else
        {
            _callable = (_callable_t*)_sbob();
            rhs._callable->move(_sbob());
            rhs._destroy();
        }
    }

   public:
    function& operator=(function const& fn) noexcept = delete;
    function(function const& fn) noexcept            = delete;

    Ret_ operator()(Args_... args)
    {
        if (_callable == nullptr)
            throw std::bad_function_call{};

        return std::invoke(*_callable, std::forward<Args_>(args)...);
    }

    template <
            typename Callable_,
            typename = std::enable_if_t<
                    not std::is_same_v<
                            function,
                            std::remove_cv_t<std::remove_reference_t<Callable_>>>>>
    function& operator=(Callable_&& fn) noexcept(std::is_nothrow_move_constructible_v<Callable_>)
    {
        _destroy();
        _assign_function(std::forward<Callable_>(fn));
        return *this;
    }

    function& operator=(function&& fn) noexcept
    {
        _destroy();
        _move_from(std::move(fn));
        return *this;
    }

    template <
            typename Callable_,
            typename = std::enable_if_t<
                    not std::is_same_v<
                            function,
                            std::remove_cv_t<std::remove_reference_t<Callable_>>>>>
    function(Callable_&& fn) noexcept(std::is_nothrow_move_constructible_v<Callable_>)
    {
        _assign_function(std::forward<Callable_>(fn));
    }

    function(function&& fn) noexcept
    {
        _move_from(std::move(fn));
    }

    function() noexcept = default;

    ~function() noexcept
    {
        _destroy();
    }

    bool is_sbo() const noexcept
    {
        return _is_sbo();
    }

   private:
    void _destroy()
    {
        if (_is_sbo())
            (*_callable).~_callable_t();
        else
            delete _callable;

        _callable = nullptr;
    }

    bool _is_sbo() const noexcept
    {
        return (void*)_callable == (void*)_sbob();
    }

    void* _sbob() const noexcept
    {
        return const_cast<char*>(_sbo_buf.data());
    }

    void _static_assert_expr() const noexcept
    {
        static_assert(sizeof _sbo_buf == _function_size - sizeof(void*));
        static_assert(sizeof(function) == _function_size);
    }

   private:
    _callable_t* _callable                                    = nullptr;
    std::array<char, _function_size - sizeof(void*)> _sbo_buf = {};
};

}  // namespace CPPHEADERS_NS_
