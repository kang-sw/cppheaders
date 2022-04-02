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
#include "../../__namespace__"
#include "session.hxx"

namespace CPPHEADERS_NS_::rpc {
/**
 * Builds a session.
 *
 * @tparam SlotValue A compile-time flag for verifying correct creation of session
 */
template <int SlotValue>
class basic_session_builder
{
    enum slot_flags {
        flag_initialized,

        slot_event_proc,
        slot_connection,
        slot_protocol,

        opt_slot_user_data,
        opt_slot_monitor,
        opt_slot_service,
    };

   private:
    session_ptr _session;

   private:
    template <slot_flags... Values,
              typename = std::enable_if_t<
                      (SlotValue & (1 << flag_initialized))
                      && not(SlotValue & ((1 << Values) | ...))>>
    auto& _make_ref() noexcept
    {
        return (basic_session_builder<((SlotValue | (1 << Values)) | ...)>&)*this;
    }

   public:
    inline auto& start()
    {
        assert(not _session);
        _session = std::make_shared<session>(session::_ctor_hide_type{});
        return (basic_session_builder<1 << flag_initialized>&)*this;
    }

    inline auto& user_data(shared_ptr<void> ptr)
    {
        assert(ptr);
        _session->_profile.user_data = std::move(ptr);
        return _make_ref<opt_slot_user_data>();
    }

    inline auto& monitor(shared_ptr<if_session_monitor> ptr)
    {
        assert(ptr);
        _session->_monitor = std::move(ptr);
        return _make_ref<opt_slot_monitor>();
    }

    inline auto& service(service ptr)
    {
        assert(ptr);
        _session->_service = std::move(ptr);
        return _make_ref<opt_slot_monitor>();
    }

    inline auto& event_procedure(shared_ptr<if_event_proc> ptr)
    {
        assert(ptr);
        _session->_event_proc = std::move(ptr);
        return _make_ref<slot_event_proc>();
    }

    inline auto& connection(unique_ptr<if_connection_streambuf> ptr)
    {
        assert(ptr);
        _session->_conn = std::move(ptr);
        return _make_ref<slot_connection>();
    }

    inline auto& protocol(unique_ptr<if_protocol_stream> ptr)
    {
        assert(ptr);
        _session->_protocol = std::move(ptr);
        return _make_ref<slot_protocol>();
    }

    [[nodiscard]] inline auto
    build()
    {
        static_assert(SlotValue & (1 << slot_event_proc));
        static_assert(SlotValue & (1 << slot_connection));
        static_assert(SlotValue & (1 << slot_protocol));

        if constexpr (not(SlotValue & (1 << opt_slot_monitor)))
            _session->_monitor = std::make_shared<if_session_monitor>();

        _session->_initialize();
        return std::move(_session);
    }

    inline auto build_to(session_ptr& out)
    {
        out.reset();
        out = build();
    }
};
}  // namespace CPPHEADERS_NS_::rpc
