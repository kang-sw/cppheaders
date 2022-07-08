
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
#include <cpph/std/map>
#include <optional>

#include "../../../streambuf/string.hxx"
#include "../../archive/json-writer.hxx"
#include "interface.hxx"
#include "service.hxx"

namespace cpph::rpc {
class remote_procedure_message_proxy
{
    friend class session;

    enum class proxy_type {
        none,
        in_progress,

        request,
        notify,
        reply_okay,
        reply_error,

        reply_expired,
    };

   private:
    if_session* _owner = {};
    service* _svc = {};

    proxy_type _type = proxy_type::none;
    int _rpc_msgid = 0;

    //
    std::optional<service_handler_package> _handler;

   public:
    /**
     * Find corresponding request parameter buffer with given method name,
     *  and set internal state as request mode
     */
    service_parameter_buffer* request_parameters(string_view method_name, int msgid)
    {
        auto rv = _parameters(method_name);

        if (rv) {
            _type = proxy_type::request;
            _rpc_msgid = msgid;
        }

        return rv;
    }

    /**
     * Find corresponding notify handler parameter buffer with given method name,
     *  and set internal state as notify mode
     */
    service_parameter_buffer* notify_parameters(string_view method_name)
    {
        auto rv = _parameters(method_name);

        if (rv) {
            _type = proxy_type::notify;
        }

        return rv;
    }

    /**
     * Retrieves result buffer of waiting RPC
     */
    bool reply_result(int msgid, archive::if_reader* object)
    {
        _verify_clear_state();

        _type = proxy_type::reply_expired;
        _rpc_msgid = msgid;

        _owner->request_node_lock_begin();
        cleanup_t _cleanup{[&] { _owner->request_node_lock_end(); }};

        auto rval = _owner->find_reply_result_buffer(msgid);
        if (rval == nullptr) {
            *object >> nullptr;
            return false;
        }

        if (rval->empty())  // Handle void-return buffer.
            *object >> nullptr;
        else
            *object >> *rval;

        _type = proxy_type::reply_okay;
        return true;
    }

    /**
     * Set reply as error, and dump single object as error
     */
    bool reply_error(int msgid, archive::if_reader* object)
    {
        _verify_clear_state();

        _type = proxy_type::reply_expired;
        _rpc_msgid = msgid;

        _owner->request_node_lock_begin();
        cleanup_t _cleanup{[&] { _owner->request_node_lock_end(); }};

        auto json = _owner->find_reply_error_buffer(msgid);
        if (json == nullptr) {
            *object >> nullptr;
            return false;
        }

        streambuf::stringbuf buf{json};
        archive::json::writer writer(&buf);

        object->dump_single_object(&writer);

        _type = proxy_type::reply_error;
        return true;
    }

   private:
    void _verify_clear_state()
    {
        assert(_type == proxy_type::none);
        assert(_rpc_msgid == 0);
    }

    service_parameter_buffer* _parameters(string_view method_name)
    {
        _verify_clear_state();
        _type = proxy_type::in_progress;

        auto handler = _svc->find_handler(method_name);
        if (not handler) { return nullptr; }

        _handler = handler->checkout_parameter_buffer();
        return &_handler->params;
    }
};

}  // namespace cpph::rpc