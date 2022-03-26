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
#include <memory>
#include <set>
#include <vector>

#include "../__namespace__"
#include "../algorithm/std.hxx"
#include "../utility/cleanup.hxx"
#include "../utility/singleton.hxx"
#include "event_wait.hxx"
#include "locked.hxx"
#include "thread_pool.hxx"

/**
 * Defines pipeline
 */
namespace CPPHEADERS_NS_::pipe {
using std::atomic;
using std::set;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;
using std::weak_ptr;
using std::chrono::microseconds;

template <typename InputType, typename SharedDataType>
class pipe;

namespace detail {

struct pipe_sequence_context
{
    size_t           id;
    shared_ptr<void> shared_data;
};

class root_pipe
{
    mutable thread::event_wait _inputs_lock;

   public:
    auto& event_wait() const { return _inputs_lock; }
};

/**
 * TODO: 파이프라인의 실행 흐름
 *  1. 루트 파이프에 데이터 삽입 -> 비동기 프로시져 대기
 *  2. 프로시저 종료, 프로시저 내에서 자체 다음 파이프 링크에 데이터 커밋
 *     다음 파이프 문맥에서 ...
 *  2.1. 대기중인 fence보다 수신한 fence의 값이 더 작음 -> 버린다.
 *       더 큼 -> 대체한다 -> 같게 됨
 *  2.2. 버퍼 복사
 *  2.3. num_links_left decrement.
 *          num_links_left > 0 -> 아무것도 하지 않음
 *          num_links_left == 0 -> 이벤트 프로시저 포스트
 *  3.1. 포스팅 된 프로시저 내부에서 다시 2. 반복
 */

class pipe_base : std::enable_shared_from_this<pipe_base>
{
    set<weak_ptr<pipe_base>, std::owner_less<>> _inputs;
    std::type_info const*                       _shared_type_id;

    //
    shared_ptr<pipe_sequence_context const> _current_fence;  // immutable while in procedure
    std::mutex                              _inputs_lock;
    std::mutex                              _procedure_lock;

    //
    size_t _next_fence_id = 0;
    size_t _num_input_left = 0;

   public:
    explicit pipe_base(std::type_info const* shared_type_compat) noexcept : _shared_type_id(shared_type_compat) {}
    virtual ~pipe_base() = default;

   public:
    void reset_input() { _inputs.clear(); }

    void add_input(shared_ptr<pipe_base> ptr)
    {
        if (ptr->_shared_type_id != _shared_type_id)
            throw std::logic_error{"Shared data type is not compatible!"};

        lock_guard lc_{_inputs_lock};
        _inputs.insert(std::move(ptr));
    }

   protected:
    bool _check_fence_(detail::pipe_base* caller)
    {
        if (not contains(_inputs, caller->weak_from_this()))
            throw std::logic_error{"Input is not set!"};

        auto const& caller_fence = caller->_current_fence;

        if (caller_fence->id < _next_fence_id)
            return false;  // This input is already discarded

        if (caller_fence->id > _next_fence_id) {
            _next_fence_id = caller_fence->id;
            _num_input_left = _inputs.size();
        }

        return true;
    }

    bool _try_post_procedure_(detail::pipe_base* caller)
    {
        // validate once more, fence could be invalidated
        auto const& caller_fence = caller->_current_fence;
        if (caller_fence->id < _next_fence_id)
            return false;

        if (--_num_input_left > 0)
            return false;  // do nothing.

        // wait until any active procedure finishes
        std::unique_lock lc_{_procedure_lock};

        // post procedure
        _current_fence = caller->_current_fence;
        _swap_value_buffer();

        auto fn = bind_front_weak(weak_from_this(), &pipe_base::_procedure_, this, std::move(lc_));
        default_singleton<thread_pool>().post(std::move(fn));

        return true;
    }

    void _wait_procedure_()
    {
        lock_guard lc_{_procedure_lock};
    }

    auto _input_guard_()
    {
        return std::unique_lock{_inputs_lock};
    }

    void* _shared_data_()
    {
        return _current_fence->shared_data.get();
    }

   private:
    void _procedure_(std::unique_lock<std::mutex>)
    {
        // Invoke procedure
        _invoke_procedure_();

        // Clear current fence
        _current_fence = {};
    }

    virtual void _swap_value_buffer() = 0;
    virtual void _invoke_procedure_() = 0;

   public:
    void _verify_called_inside_procedure_() {}
};
}  // namespace detail


template <typename InputType, typename SharedDataType>
class pipe : protected detail::pipe_base
{
    InputType _buf[2];
    int       _bufidx = 0;

   public:
    pipe() : pipe_base(&typeid(SharedDataType)) {}

    /**
     * Pipe class have to make call to next pipe input before exit procedure
     * @param input
     */
    virtual void procedure(InputType& input, SharedDataType& shared) = 0;

   protected:
    /**
     * Make call to
     * @tparam Callable
     * @param caller
     * @param set_param
     */
    template <typename Callable>
    void commit(detail::pipe_base* caller, Callable&& set_param)
    {
        caller->_verify_called_inside_procedure_();

        auto lock = this->_input_guard_();
        if (not _check_fence_(caller))
            return;

        set_param(_buf[_bufidx ^ 1]);

        // Wait for all procedure finish
        lock.unlock();
        _wait_procedure_();

        lock.lock();
        _try_post_procedure_(caller);  // on successful post,
    }

   private:
    void _swap_value_buffer() override
    {
        _bufidx ^= 1;
    }

    void _invoke_procedure_() override
    {
        procedure(_buf[_bufidx], *(SharedDataType*)_shared_data_());
    }
};
}  // namespace CPPHEADERS_NS_::pipe
