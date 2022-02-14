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
#include "msgpack-rpc/context.hxx"

/**
 * Provides basic I/O functionality of msgpack-rpc
 *
 * \code
 *   // can represent receive, send both
 *   class my_connection : refl::msgpack::basic_rpc_connection {
 *     // constructor interface
 *     my_connection(int a, int b);
 *
 *     // necessary overrides
 *     bool initialize() override
 *     {
 *       auto n_next = basic_rpc_connection::recv_next;
 *       _impl->async_recv(&my_connection::_on_recv);
 *     }
 *
 *     size_t send_data(array_view<void const> data) override
 *     {
 *       return _impl->send_data(data);
 *     }
 *
 *     size_t recv_data(array_view<void> buffer) override
 *     {
 *       return _impl->recv_data(buffer.data(), buffer.size());
 *     }
 *
 *     // call for basic class
 *    private:
 *     void _on_recv()
 *     {
 *       basic_rpc_connection::wakeup();
 *     }
 *
 *     void _on_error()
 *     {
 *       // unregister self from context
 *       basic_rpc_connection::disconnected();
 *     }
 *   };
 *
 *   ... // server
 *   refl::msgpack::rpc_context ctx;
 *
 *   // does not distinguish request/notify.
 *   function<std::string(int, std::string)> handler = ...;
 *   ctx.serve("method_name", std::move(handler));
 *
 *   ... // client
 *   // if there's multiple connections available, ctx assumes they share same interface, thus
 *   //  it simply load-balance and broadcast identical contents within connections.
 *   my_msgpack_rpc_context ctx;
 *   my_connection conn{"localhost:1222"}; // must be alive
 *   ctx.add_connection<my_connection>("hello, world!");
 *   ctx.setopt(refl::msgpack::rpcopt::timeout, 10.);
 *
 *   // synchronous method. throws on error.
 *
 *   // rpc request. load-balanced automatically. (round-robin).
 *   // execution order is not guaranteed.
 *   //   errors can be any of the next:
 *   //     method_not_exist
 *   //     connection_timeout
 *   //     invalid_signature
 *   //       invalid_parameter  // failed to parse parameter pack
 *   //       invalid_return_type
 *   //     unkown_error // from other library ... error info is incompatible
 *   //
 *   // if method return type is void, it simply wait for server reply.
 *   std::string result = ctx.request<std::string>("method_name", 3, "hello!");
 *
 *   // async
 *   std::future<std::string> async_result = ctx.async_request<std::string>(...);
 *
 *   // notify single/all.
 *   // on single, you can't specify which connection should be notified.
 *   //  basically it balances load based on round-robin algorithm.
 *   // even though served rpc in server returns value, it will silently be discarded if
 *   //  rpc mode was notify.
 *   ctx.notify("method_name", 1, "hell!);
 *   ctx.notify_all("method_name", 2, "o, world!);
 *
 * \endcode
 */
