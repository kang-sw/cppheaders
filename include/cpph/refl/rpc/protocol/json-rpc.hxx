#pragma once

#include "../../archive/json.hpp"
#include "../../detail/primitives.hxx"
#include "../detail/protocol_procedure.hxx"

namespace cpph::rpc::protocol {

/**
 * - Does not support batch RPC request
 * - Supports jsonrpc-v2 partially
 *
 */
class jsonrpc : public if_protocol_procedure
{
    enum class msgtype {
        request,
        reply,
        notify
    };

   public:
    void initialize(std::streambuf* streambuf) override
    {
    }

    protocol_stream_state handle_single_message(remote_procedure_message_proxy& proxy) noexcept override
    {
        return protocol_stream_state::warning_unknown;
    }

    bool send_request(std::string_view method, int msgid, array_view<refl::object_const_view_t> params) noexcept override
    {
        return false;
    }

    bool send_notify(std::string_view method, array_view<refl::object_const_view_t> params) noexcept override
    {
        return false;
    }

    bool flush() noexcept override
    {
        return false;
    }

    bool send_reply_result(int msgid, refl::object_const_view_t retval) noexcept override
    {
        return false;
    }

    bool send_reply_error(int msgid, refl::object_const_view_t error) noexcept override
    {
        return false;
    }

    bool send_reply_error(int msgid, std::string_view content) noexcept override
    {
        return false;
    }
};
}  // namespace cpph::rpc::protocol