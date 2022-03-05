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

#include <thread>

#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/steady_timer.hpp>

#include "catch.hpp"
#include "refl/extension/msgpack-rpc.hxx"
#include "refl/extension/msgpack-rpc/asio.hxx"
#include "refl/object.hxx"

using namespace std::literals;
using namespace cpph;

TEST_CASE("Read socket zero receive", "[asio][.]")
{
    using namespace asio;
    using asio::ip::tcp;

    io_context ioc;
    tcp::acceptor acpt{ioc};
    tcp::endpoint ep{ip::make_address("127.0.0.1"), 34561};
    acpt.open(ep.protocol());

    acpt.set_option(tcp::acceptor::reuse_address{true});
    acpt.bind(ep);

    acpt.listen();

    steady_timer tim{ioc};

    bool written = false;
    tcp::socket sock{ioc};
    acpt.async_accept(
            sock,
            [&](error_code ec) {
                if (ec) { FAIL(); }

                WARN("Server: Accepted New Connection");
                tim.expires_after(1s);
                tim.async_wait([&](asio::error_code) {
                    WARN("Server: Writing Data");

                    asio::basic_socket_streambuf buf{std::move(sock)};

                    std::this_thread::sleep_for(1s);
                    buf.sputn("hello!", 6);
                    buf.pubsync();

                    written = true;
                });
            });

    tcp::socket recv{ioc};
    char buf2[6];

    recv.open(ep.protocol());
    recv.async_connect(
            ep,
            [&](error_code ec) {
                if (ec) { FAIL(ec.value()); }

                CHECK(not written);
                WARN("Client: Connection Successful");

                char buf[1];
                recv.async_wait(
                        tcp::socket::wait_read,
                        [&](error_code ec2) {
                            if (ec2) { FAIL(); }

                            WARN("Client: Start receiving data");
                            async_read(
                                    recv, asio::buffer(buf2), transfer_all(),
                                    [&](error_code ec3, size_t n3) {
                                        CHECK(written);
                                        REQUIRE(n3 == 6);
                                        WARN("Client: Received 6 bytes");
                                    });
                        });
            });

    ioc.run();
}

TEST_CASE("Tcp context", "[msgpack-rpc][.]")
{
    using namespace asio;
    using namespace msgpack::rpc::asio;

    using asio::ip::tcp;

    function<void(msgpack::rpc::if_connection const*, int*, int)> fn;
    fn = [](auto ptr, int* rv, int val) {
        WARN("Peer [" << ptr->peer() << "]: " << val);
        *rv = val * val;
    };

    msgpack::rpc::service_info service;
    service.serve_full("hello", std::move(fn));

    io_context ioc;
    auto ctx = create_rpc_context(ioc, service);

    tcp::acceptor acpt{ioc};
    tcp::endpoint ep{ip::make_address("127.0.0.1"), 34561};
    acpt.open(ep.protocol());
    acpt.bind(ep);

    msgpack::rpc::session_config cfg;
    open_acceptor(*ctx, cfg, acpt);
    WARN("Server: Acceptor now open");

    tcp::socket client{ioc};
    client.open(ep.protocol());
    client.connect(ep);

    WARN("Client: Connection successful");

    auto hsession = create_session(*ctx, std::move(client));
    WARN("Client: Session created");

    SECTION("Basic disconnection")
    {
        ioc.restart();
        ioc.run_for(1s);
        REQUIRE(ctx->session_count() == 2);

        ctx->erase_session(hsession);

        ioc.restart();
        ioc.run_for(1s);

        CHECK(not hsession);
    }

    ioc.restart();
    std::thread worker_ioc{[&ioc] { ioc.run(); }};

    SECTION("Basic RPC")
    {
        int rv = 0;
        ctx->rpc(&rv, "hello", 3);
        REQUIRE(rv == 9);
    }

    ioc.stop();
    worker_ioc.join();
}
