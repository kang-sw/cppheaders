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
#undef NDEBUG

//
#include "refl/object.hxx"
#include "refl/rpc/connection/inmemory_pipe.hxx"
#include "refl/rpc/default_event_procedure.hxx"
#include "refl/rpc/protocol/msgpack-rpc.hxx"
#include "refl/rpc/rpc.hxx"
#include "refl/rpc/service.hxx"
#include "refl/rpc/session_builder.hxx"
#include "thread/message_procedure.hxx"
using namespace cpph;

//
#if __has_include("asio.hpp")
#    include "asio/ip/tcp.hpp"
#    include "refl/rpc/connection/asio.hxx"
#endif

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

#if __has_include("asio.hpp")
    using asio::ip::tcp;
    asio::io_context ioc;
    auto work = asio::require(ioc.get_executor(), asio::execution::outstanding_work.tracked);

    tcp::socket sock1{ioc};
    tcp::socket sock2{ioc};

    tcp::acceptor acpt{ioc};
    auto acpt_ep = tcp::endpoint{asio::ip::make_address("0.0.0.0"), 5151};
    acpt.open(acpt_ep.protocol());
    acpt.set_option(asio::socket_base::reuse_address{true});
    acpt.bind(acpt_ep);

    volatile bool ready = false;
    acpt.listen();
    acpt.async_accept(sock1,
                      [&](auto&& ec) {
                          printf("sock1 open, %s:%d ...\n",
                                 sock1.remote_endpoint().address().to_string().c_str(),
                                 sock1.remote_endpoint().port());
                          ready = true;
                      });

    auto conn_ep = tcp::endpoint{asio::ip::make_address("127.0.0.1"), 5151};
    sock2.open(conn_ep.protocol());
    sock2.connect(conn_ep);

    printf("sock2 open, %s:%d ...\n",
           sock2.remote_endpoint().address().to_string().c_str(),
           sock2.remote_endpoint().port());
    fflush(stdout);

    std::thread ioc_thread{[&] { ioc.run(); }};
    while (not ready) { std::this_thread::yield(); }
    //
    auto conn_b = std::make_unique<rpc::asio_stream<tcp>>(std::move(sock2));
    auto conn_a = std::make_unique<rpc::asio_stream<tcp>>(std::move(sock1));

    auto event_proc = std::make_shared<rpc::asio_event_procedure<asio::io_context>>(ioc);
#else
    auto [conn_a, conn_b] = rpc::conn::inmemory_pipe::create();
    auto event_proc = rpc::default_event_procedure::get();
#endif

    rpc::session_ptr session_server;
    rpc::session_ptr session_client;

    rpc::session::builder{}
            .connection(std::move(conn_a))
            .service(service)
            .protocol(std::make_unique<rpc::protocol::msgpack>())
            //            .event_procedure(std::make_shared<asio_event_proc>())
            .event_procedure(event_proc)
            .build_to(session_server);

    rpc::session::builder{}
            .enable_request()
            .connection(std::move(conn_b))
            .protocol(std::make_unique<rpc::protocol::msgpack>())
            //            .event_procedure(std::make_shared<asio_event_proc>())
            .event_procedure(event_proc)
            .build_to(session_client);

    SECTION("Blocking request")
    {
        sg_add(session_client).notify(1, 2 * 3);

        auto val = sg_add(session_client).request(1, 4);
        REQUIRE(val == 5);
        val = sg_add(session_client).request(5, 2);
        REQUIRE(val == 7);

        for (int i = 0; i < 8192; ++i) {
            val = sg_add(session_client).request(i, i * i);
            REQUIRE(val == i * i + i);

            sg_add(session_client).notify(1, 2 * 3);

            auto str = sg_concat(session_client).request("1", "2");
            REQUIRE(str == "12");
        }
    }

    SECTION("Non-blocking request")
    {
        std::list<std::pair<int, int>> retbufs;
        std::list<rpc::request_handle> handles;

        for (int i = 0; i < 32767; ++i) {
            auto rbuf = &retbufs.emplace_back();
            auto h = sg_add(session_client).async_request(&rbuf->first, i << 24, i * i);
            rbuf->second = (i << 24) + i * i;

            handles.emplace_back(h);
        }

        while (not retbufs.empty()) {
            auto& h = handles.front();

            REQUIRE(h.wait());
            REQUIRE(retbufs.front().first == retbufs.front().second);

            retbufs.pop_front(), handles.pop_front();
        }
    }

#if __has_include("asio.hpp")
    ioc.stop();
    ioc_thread.join();

    size_t nrd, nwr;
    session_server->totals(&nrd, &nwr);
    printf("total read: %zu, write: %zu\n", nrd, nwr);
    fflush(stdout);
#endif
}

TEST_CASE("Inmemory Pipe Test", "[rpc]")
{
    auto [conn_a, conn_b] = rpc::conn::inmemory_pipe::create();
    char const content[] = "hello, world!";

    for (size_t i = 0; i < 1024; ++i) {
        conn_a->sputn(content, strlen(content));
        conn_a->pubsync();

        char buf[sizeof(content)] = {};
        conn_b->sgetn(buf, strlen(content));

        REQUIRE(strcmp(content, buf) == 0);
    }
}
