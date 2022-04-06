/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2022. Seungwoo Kang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * project home: https://github.com/perfkitpp
 ******************************************************************************/

#pragma once

#include <memory>

#include "../../../functional.hxx"
#include "../../__namespace__"
#include "../../detail/object_core.hxx"
#include "defs.hxx"

namespace CPPHEADERS_NS_::rpc {
using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;

class remote_procedure_message_proxy;

class if_event_proc
{
   public:
    virtual ~if_event_proc() = default;

    /**
     * Post RPC completion event.
     *
     * Low priority.
     */
    virtual void post_rpc_completion(function<void()>&& fn) { post_internal_message(std::move(fn)); }

    /**
     * Post incoming request/notify handler callback. Median priority.
     */
    virtual void post_handler_callback(function<void()>&& fn) { post_internal_message(std::move(fn)); };

    /**
     * Post internal messages. High priority.
     */
    virtual void post_internal_message(function<void()>&& fn) = 0;
};

/**
 * Indicates single handler
 */
class if_service_handler
{
   public:
    struct handler_package_type {
        shared_ptr<if_service_handler>  _self;
        shared_ptr<void>                _param_body;
        array_view<refl::object_view_t> params;

       public:
        refl::shared_object_ptr invoke(session_profile const& profile) &&
        {
            return _self->invoke(profile, std::move(*this));
        }
    };

   public:
    virtual ~if_service_handler() = default;

    /**
     * Returns parameter buffer of this handler.
     */
    virtual auto checkout_parameter_buffer() -> handler_package_type = 0;

   private:
    /**
     * Invoke handler with given parameters, and return invocation result.
     */
    virtual auto invoke(session_profile const&, handler_package_type&& params)
            -> refl::shared_object_ptr = 0;
};

using service_handler_package = if_service_handler::handler_package_type;
using service_parameter_buffer = decltype(if_service_handler::handler_package_type::params);

/**
 * All events can be invoked during multithreading context!
 */
class if_session_monitor
{
   public:
    virtual ~if_session_monitor() = default;

    /**
     * Invoked when session is expired
     */
    virtual void on_session_expired(session_profile_view) {}

    /**
     * Invoked when session created
     */
    virtual void on_session_created(session_profile_view) {}

    /**
     * Invoked when recoverable error occurred during receiving
     */
    virtual void on_receive_warning(session_profile_view, protocol_stream_state) {}

    /**
     * On error occurred during handler invocation
     */
    virtual void on_handler_error(session_profile_view, std::exception& e) {}
};

/**
 * Session interface
 */
class if_session
{
    friend class if_connection;
    friend class remote_procedure_message_proxy;

   public:
    virtual ~if_session() = default;

   private:
    virtual void on_data_wait_complete() noexcept = 0;

    // RPC functions
    virtual void request_node_lock_begin() = 0;
    virtual void request_node_lock_end() = 0;

    virtual auto find_reply_result_buffer(int msgid) -> refl::object_view_t* = 0;
    virtual auto find_reply_error_buffer(int msgid) -> std::string* = 0;
};

}  // namespace CPPHEADERS_NS_::rpc
