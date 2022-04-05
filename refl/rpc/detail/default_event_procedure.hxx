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
#include "../../../thread/thread_pool.hxx"
#include "../../../utility/singleton.hxx"
#include "interface.hxx"

namespace CPPHEADERS_NS_::rpc {
class default_event_procedure : public if_event_proc
{
   public:
    void post_rpc_completion(function<void()>&& fn) override
    {
        default_singleton<thread_pool>().post(std::move(fn));
    }

    void post_handler_callback(function<void()>&& fn) override
    {
        default_singleton<thread_pool>().post(std::move(fn));
    }

    void post_internal_message(function<void()>&& fn) override
    {
        default_singleton<thread_pool>().post(std::move(fn));
    }

   public:
    static auto get() noexcept
    {
        static auto _value = std::make_shared<default_event_procedure>();
        return _value;
    }
};

}  // namespace CPPHEADERS_NS_::rpc