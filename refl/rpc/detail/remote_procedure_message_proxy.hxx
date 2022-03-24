
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
#include <map>
#include <optional>

#include "../../__namespace__"
#include "interface.hxx"

namespace CPPHEADERS_NS_::rpc {
struct packed_service_handler
{
    friend class remote_procedure_message_proxy;

   private:
    if_service_handler::handler_package_type _buf;

   public:
    auto parameter_buffer() const noexcept { return _buf.params; }
};

class remote_procedure_message_proxy
{
    friend class session;

    enum class proxy_type {
        none,
        request,
        notify,
        reply_okay,
        reply_error
    };

   private:
    service*       _svc = {};
    if_event_proc* _procedure = {};

    proxy_type     _type = proxy_type::none;
    int            _rpc_msgid = 0;

   public:
    /**
     * Find service handler with method name.
     *
     * @return empty optional if method not found.
     */
    std::optional<packed_service_handler> find_handler(string_view method_name);

    /**
     * Post notify handler to event procedure
     */
    void post_notify_handler(packed_service_handler&& deserialized);

    /**
     * Post RPC handler to event procedure
     */
    void post_rpc_handler(int msgid, packed_service_handler&& deserialized);

    /**
     * Retrieves result buffer of waiting RPC
     */
    refl::object_view_t find_reply_buffer(int msgid);

    /**
     * Set reply as error, and get error buffer
     */
    string* set_reply_as_error(int msgid);
};

}  // namespace CPPHEADERS_NS_::rpc