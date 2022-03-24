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

class if_event_proc
{
   public:
    virtual ~if_event_proc() = default;

    virtual void post_user_event(function<void()>&& fn) { post_system_event(std::move(fn)); }
    virtual void post_system_event(function<void()>&&) = 0;
};

class if_session
{
   public:
    virtual ~if_session() = default;

    virtual void on_data_in() = 0;
};

class if_service_handler
{
   public:
    struct parameter_buffer_type
    {
        shared_ptr<if_service_handler>  _self;
        shared_ptr<void>                _handle;

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
    virtual auto checkout_parameter_buffer() -> parameter_buffer_type = 0;

   private:
    /**
     * Invoke handler with given parameters, and return invocation result.
     */
    virtual auto invoke(session_profile const&, parameter_buffer_type&& params)
            -> refl::shared_object_ptr = 0;
};

}  // namespace CPPHEADERS_NS_::rpc
