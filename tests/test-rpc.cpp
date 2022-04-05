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
#include "refl/rpc/connection/inmemory_pipe.hxx"
#include "refl/rpc/default_event_procedure.hxx"
#include "refl/rpc/protocol/msgpack-rpc.hxx"
#include "refl/rpc/rpc.hxx"
#include "refl/rpc/service.hxx"
#include "refl/rpc/session_builder.hxx"

using namespace cpph;

TEST_CASE("Basic RPC Test", "[rpc]")
{
    using std::string;

    auto sg_add = rpc::create_signature<int(int, int)>("add");
    auto sg_concat = rpc::create_signature<string(string, string)>("concat");

    auto service = rpc::service::empty_service();
    rpc::service_builder{}
            .route(sg_add, std::plus<int>{})
            .route(sg_concat, std::plus<string>{})
            .build_to(service);

    rpc::session_ptr session_server;
    rpc::session_ptr session_client;

    auto [conn_a, conn_b] = rpc::connection::inmemory_pipe::create();

    rpc::session::builder{}
            .connection(std::move(conn_a))
            .service(service)
            .protocol(std::make_unique<rpc::protocol::msgpack>())
            .event_procedure(rpc::default_event_procedure::get())
            .build_to(session_server);

    rpc::session::builder{}
            .enable_request()
            .connection(std::move(conn_b))
            .protocol(std::make_unique<rpc::protocol::msgpack>())
            .event_procedure(rpc::default_event_procedure::get())
            .build_to(session_client);

    SECTION("Blocking request")
    {
        sg_add(session_client).notify(1, 2 * 3);

        auto val = sg_add(session_client).request(1, 4);
        REQUIRE(val == 5);
        val = sg_add(session_client).request(5, 2);
        REQUIRE(val == 7);

        for (int i = 0; i < 4096; ++i) {
            val = sg_add(session_client).request(i, i * i);
            REQUIRE(val == i * i + i);

            auto str = sg_concat(session_client).request("1", "2");
            REQUIRE(str == "12");
        }
    }

    SECTION("Non-blocking request")
    {
        std::list<int>                 retbufs;
        std::list<rpc::request_handle> handles;
    }
}

TEST_CASE("Inmemory Pipe Test", "[rpc]")
{
    auto [conn_a, conn_b] = rpc::connection::inmemory_pipe::create();
    char const content[] = "hello, world!";

    for (size_t i = 0; i < 1024; ++i) {
        conn_a->sputn(content, strlen(content));
        conn_a->pubsync();

        char buf[sizeof(content)] = {};
        conn_b->sgetn(buf, strlen(content));

        REQUIRE(strcmp(content, buf) == 0);
    }
}
