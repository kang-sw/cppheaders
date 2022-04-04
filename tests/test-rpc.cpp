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

#include "catch.hpp"

//
#include "refl/object.hxx"
#include "refl/rpc/connection.hxx"
#include "refl/rpc/connection/inmemory_pipe.hxx"
#include "refl/rpc/detail/protocol_stream.hxx"
#include "refl/rpc/detail/service.hxx"
#include "refl/rpc/detail/service_builder.hxx"
#include "refl/rpc/detail/session.hxx"
#include "refl/rpc/detail/session_builder.hxx"
#include "refl/rpc/protocol.hxx"
#include "refl/rpc/protocol/msgpack-rpc.hxx"
#include "refl/rpc/service.hxx"

using namespace cpph;

TEST_CASE("Can compile modules", "[rpc][.]")
{
    auto sig1 = rpc::create_signature<int(int, bool)>("hello");
    auto sig2 = rpc::create_signature<int(int, bool, std::string)>("hello");
    auto sig3 = rpc::create_signature<double(double)>("hello");
    auto svc = rpc::service_builder{};

    sig3.wrap([](auto, double*, double&) {});
    svc.route(sig3, [](double) -> double { return 0; });
    svc.route(sig3, [](double*, double) { return 0; });
    svc.route(sig1, [](int*, int&, bool&) {});
    svc.route(sig2, [](rpc::session_profile_view, int*, int&, bool&, std::string&) {});

    auto dr = [](double) -> double { return 0; };
    static_assert(std::is_invocable_r_v<double, decltype(dr), double>);

    decltype(sig3)::serve_signature_0 srr = [](double&) -> double { return 0; };
    decltype(sig1)::return_type       r;

    class proto : public rpc::if_protocol_stream
    {
       public:
        void initialize(rpc::if_connection_streambuf* streambuf) override
        {
        }
        rpc::protocol_stream_state handle_single_message(rpc::remote_procedure_message_proxy& proxy) noexcept override
        {
            return rpc::protocol_stream_state::warning_received_invalid_parameter_type;
        }
        bool send_request(std::string_view method, int msgid, array_view<refl::object_view_t> params) noexcept override
        {
            return false;
        }
        bool send_notify(std::string_view method, array_view<refl::object_view_t> params) noexcept override
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

    class conn : public rpc::if_connection_streambuf
    {
       public:
        conn(const std::string& peer_name) : if_connection_streambuf(peer_name) {}

        void initialize() noexcept override
        {
        }
        void start_data_receive() noexcept override
        {
        }
        void close() noexcept override
        {
        }
        void get_total_rw(size_t* num_read, size_t* num_write) override
        {
        }
    };

    rpc::session_ptr session;
    rpc::session::builder{}
            .user_data(nullptr)
            .event_procedure(nullptr)
            .protocol(std::make_unique<proto>())
            .service(std::move(svc).build())
            .connection(std::make_unique<conn>("hello"))
            .build_to(session);
}
