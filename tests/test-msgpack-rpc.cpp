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

#include <csignal>
#include <future>
#include <iostream>
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
    using namespace msgpack::rpc::asio_ex;

    using asio::ip::tcp;
    static std::mutex _mtx_cout;

    function<void(msgpack::rpc::session_profile_view, int*, int, std::string)> fn =
            [](auto&& profile, int* rv, int val, std::string arg2) {
                {
                    std::lock_guard _lc_{_mtx_cout};
                    printf("Peer [%s]: %d, %s\n", profile.peer_name.c_str(), val, arg2.c_str());
                    fflush(stdout);
                }

                *rv = val * val;
            };

    function<int(bool)> fn2
            = [](bool value) {
                  if (value)
                      return 264;
                  else
                      throw std::runtime_error{"Vlue!"};
              };

    auto stub_print = msgpack::rpc::create_signature<void(std::string)>("print");
    auto stub_noti  = msgpack::rpc::create_signature<void(void)>("noti");

    msgpack::rpc::service_info service;
    service.serve2("hello", std::move(fn));
    service.serve("except", std::move(fn2));
    service(stub_print, [](auto&& str) { printf("hello, world! %s\n", str.c_str()); });
    service(stub_noti, [] { printf("noti!\n"); });

    io_context ioc;
    auto ctx = create_rpc_context(ioc, service);

    tcp::acceptor acpt{ioc};
    tcp::endpoint ep{ip::make_address("127.0.0.1"), 34561};
    acpt.open(ep.protocol());
    acpt.set_option(tcp::acceptor::reuse_address{true});
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

    if (0)
        SECTION("Basic disconnection")
        {
            ioc.restart();
            ioc.run_for(200ms);
            REQUIRE(ctx->session_count() == 2);

            ctx->erase_session(hsession);

            ioc.restart();
            ioc.run_for(200ms);

            CHECK(not hsession);
        }

    ioc.restart();

    SECTION("Multithreaded")
    {
        std::vector<std::thread> threads;

        threads.emplace_back([&ioc] { ioc.run(); });
        auto dup_ioc =
                [&] {
                    for (auto e : counter(std::thread::hardware_concurrency() - 1))
                        threads.emplace_back([&ioc] { ioc.run(); });
                };

        SECTION("Notify")
        {
            for (int i = 0; i < 32; ++i)
            {
                ioc.post([&ctx, i] {
                    {
                        std::lock_guard _lc_{_mtx_cout};
                        printf("Notify %d\n", i);
                        fflush(stdout);
                    }
                    ctx->notify("hello", i, "fdas");
                });
            }

            for (int i = 0; i < 32; ++i)
            {
                ioc.post([&ctx, i] {
                    {
                        std::lock_guard _lc_{_mtx_cout};
                        printf("Notify %d\n", i);
                        fflush(stdout);
                    }
                    ctx->notify("print", std::string("stub 0:") + std::to_string(i));
                });
            }

            std::this_thread::sleep_for(1s);
        }

        SECTION("Single Writer")
        {
            dup_ioc();

            for (int i = 0; i < 256; ++i)
            {
                int rv   = -1;
                auto res = ctx->rpc(&rv, "hello", i, "vv32");
                CHECK(res == msgpack::rpc::rpc_status::okay);
                CHECK(rv == i * i);
            }

            for (int i = 0; i < 256; ++i)
            {
                int rv    = -1;
                auto rslt = ctx->rpc(&rv, "hello", i);
                ctx->rpc(nullptr, "hello", i);
                CHECK(rslt == msgpack::rpc::rpc_status::invalid_parameter);
            }

            for (int i = 0; i < 256; ++i)
            {
                int rv    = -1;
                auto rslt = ctx->rpc(&rv, "hello", "fea", 3.21);
                CHECK(rslt == msgpack::rpc::rpc_status::invalid_parameter);
            }

            for (int i = 0; i < 16; ++i)
            {
                auto rslt = stub_print(*ctx).rpc(nullptr, "hello!");
                CHECK(rslt == msgpack::rpc::rpc_status::okay);
            }

            for (int i = 0; i < 16; ++i)
            {
                REQUIRE_NOTHROW(stub_noti(*ctx)());
            }
        }

        SECTION("Exceptions")
        {
            int rr = 0;
            REQUIRE_THROWS(ctx->rpc(&rr, "except", false));
            REQUIRE_NOTHROW(ctx->rpc(&rr, "except", true));
            REQUIRE(rr == 264);
        }

        SECTION("Multiple Writer")
        {
            std::atomic_int max = 64;
            std::vector<std::future<bool>> futures;

            dup_ioc();

            for (int i = 0, end = max; i < end; ++i)
            {
                futures.emplace_back(
                        std::async(std::launch::async, [&, i] {
                            auto order = max.load();
                            {
                                std::lock_guard _lc_{_mtx_cout};
                                printf("RPC %d (%d)\n", i, end - order);
                                fflush(stdout);
                            }

                            int rv      = -1;
                            auto result = ctx->rpc(&rv, "hello", i, "Gvb");
                            assert(result == msgpack::rpc::rpc_status::okay);

                            {
                                std::lock_guard _lc_{_mtx_cout};
                                printf("RPC %d -> %d (%d~%d/%d)\n", i, rv, end - order, end - --max, end);
                                fflush(stdout);
                            }
                            return rv == i * i;
                        }));
            }

            std::this_thread::sleep_for(100ms);

            bool result = true;
            for (auto& e : futures) { result &= e.get(); }

            REQUIRE(result);
        }

        ioc.stop();
        for (auto& th : threads) { th.join(); }
        threads.clear();
    }
}

TEST_CASE("interop-server", "[msgpack-rpc][.]")
{
    using namespace asio;
    using namespace msgpack::rpc;

    using asio::ip::tcp;

    io_context ioc;
    tcp::acceptor acpt{ioc};
    tcp::endpoint ep{ip::make_address("127.0.0.1"), 34561};

    acpt.open(ep.protocol());
    acpt.set_option(tcp::acceptor::reuse_address{true});
    acpt.bind(ep);

    service_info service;
    auto stub_sum = create_signature<double(double, double)>("sum");
    service(stub_sum,
            [](session_profile const& profile, auto* r, auto a, auto b) {
                printf("PEER %s: %f + %f\n", profile.peer_name.c_str(), a, b);
                fflush(stdout);
                *r = a + b;
            });

    context ctx{std::move(service)};

    msgpack::rpc::session_config cfg;
    asio_ex::open_acceptor(ctx, cfg, acpt);

    ioc.run();
}
