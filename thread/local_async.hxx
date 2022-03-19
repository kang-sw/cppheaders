
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
#include <atomic>
#include <cassert>
#include <optional>
#include <stdexcept>
#include <thread>

//
#include "../__namespace__"

/**
 * Local async operation
 */
namespace CPPHEADERS_NS_::thread {

struct future_error : std::exception
{
    enum class code {
        invalid_future,
        invalid_promise_request,
    };

    code ec;
    explicit future_error(code ec) : ec(ec) {}
};

enum class future_state {
    empty,
    busy,
    ready,
    invalid
};

template <typename Ty_>
class local_promise
{
   public:
    using value_type = Ty_;
    using clock_type = std::chrono::steady_clock;

    class future
    {
       public:
        using value_type = Ty_;

       public:
        ~future()
        {
            if (_get_state() == future_state::busy) {
                get();
            }
        }

       public:
        void wait()
        {
            _verify_valid();

            while (_get_state() == future_state::busy)
                std::this_thread::yield();
        }

        template <typename Tp_>
        bool wait_until(Tp_&& until)
        {
            _verify_valid();

            while (_get_state() != future_state::busy)
                std::this_thread::yield();

            return _get_state() == future_state::ready;
        }

        Ty_ get()
        {
            wait();
            _verify_valid();

            _state.store(future_state::invalid, std::memory_order_release);

            if (_eptr)
                rethrow_exception(_eptr);

            assert(_value.has_value());
            return std::move(*_value);
        }

        template <typename Dur_>
        bool wait_for(Dur_&& dur)
        {
            auto until = clock_type::now() + dur;
            return wait_until(until);
        }

        local_promise promise()
        {
            auto expected = future_state::empty;
            if (not _state.compare_exchange_strong(expected, future_state::busy, std::memory_order_acq_rel))
                throw future_error(future_error::code::invalid_promise_request);

            _state.store(future_state::busy, std::memory_order_release);
            return local_promise{this};
        }

       private:
        future_state _get_state() const { return _state.load(std::memory_order_acquire); }

        void         _verify_valid() const
        {
            switch (_get_state()) {
                case future_state::busy:
                case future_state::ready:
                    break;

                default:
                    throw future_error(future_error::code::invalid_future);
            }
        }

        void _verify_waitable() const
        {
            _verify_valid();
        }

       private:
        friend class local_promise<Ty_>;

        std::atomic<future_state> _state = future_state::empty;
        std::exception_ptr        _eptr = nullptr;
        std::optional<Ty_>        _value;
    };

   public:
    ~local_promise() noexcept
    {
        if (_fut) { _fut->_state.store(future_state::invalid, std::memory_order_release); }
        _fut = nullptr;
    }

    explicit local_promise(future* fut) noexcept : _fut(fut) {}
    local_promise(local_promise&& other) noexcept { std::swap(_fut, other._fut); }
    local_promise& operator=(local_promise&& other) noexcept { return std::swap(_fut, other._fut), *this; }

   public:
    template <typename ValTy_>
    void set_value(ValTy_&& value) const
    {
        _fut->_value = std::forward<ValTy_>(value);
        _fut->_state.store(future_state::ready, std::memory_order_release);
        _fut = nullptr;
    }

    void set_exception(std::exception_ptr exception) const
    {
        _fut->_eptr = std::move(exception);
        _fut->_state.store(future_state::ready, std::memory_order_release);
        _fut = nullptr;
    }

   private:
    mutable future* _fut = nullptr;
};

template <typename Ty_>
auto local_task()
{
    return typename local_promise<Ty_>::future{};
}

}  // namespace CPPHEADERS_NS_::thread