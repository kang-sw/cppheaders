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
#include <future>
#include <list>
#include <map>

#include "../../../functional.hxx"
#include "../../../helper/exception.hxx"
#include "../../../thread/event_wait.hxx"
#include "../../../thread/locked.hxx"
#include "../../__namespace__"
#include "../../archive/msgpack-reader.hxx"
#include "../../archive/msgpack-writer.hxx"
#include "../../detail/object_core.hxx"

namespace CPPHEADERS_NS_::msgpack::rpc {

CPPH_DECLARE_EXCEPTION(exception, std::exception);
CPPH_DECLARE_EXCEPTION(invalid_connection, exception);

namespace detail {
class session;
}

class context;

namespace detail {
class session
{
    context* _owner;

    // A session will automatically be expired if connection is destroyed.
    std::weak_ptr<std::streambuf> _conn;

    // A thread will be created per session.

    // Writing operation is protected.
    // - When reply to RPC
    // - When send rpc request

   public:
};
}  // namespace detail

/**
 * Defines service information
 */
class service_info
{
   public:
    using service_handler_fn = function<void(reader& param, writer& retval)>;

   private:
    std::map<std::string, service_handler_fn> _handlers;

   public:
    /**
     * Optimized version of serve(). it lets handler reuse return value buffer.
     */
    template <typename Str_, typename RetVal_, typename... Params_>
    service_info& serve_2(Str_&& method_name, function<void(RetVal_*, Params_...)> handler);

    /**
     * Serve RPC service. Does not distinguish notify/request handler. If client rpc mode was
     *  notify, return value of handler will silently be discarded regardless of its return
     *  type.
     *
     * @tparam RetVal_ Must be declared as refl object.
     * @tparam Params_ Must be declared as refl object.
     * @param method_name Name of method to serve
     * @param handler RPC handler. Uses cpph::function to accept move-only signatures
     */
    template <typename Str_, typename RetVal_, typename... Params_>
    service_info& serve(Str_&& method_name, function<RetVal_(Params_...)> handler)
    {
        this->serve_2(
                std::forward<Str_>(method_name),
                [_handler = std::move(handler)]  //
                (RetVal_ * buffer, Params_ && ... args) mutable {
                    *buffer = _handler(std::forward<Params_>(args)...);
                });

        return *this;
    }
};

class context
{
   private:
    service_info _service;

    // only be used for session creation/deletion
    locked<std::list<std::weak_ptr<std::streambuf>>> _pending_connections;

   public:
    /**
     * Create new context with given service information.
     * Once service is registered, it becomes read-only.
     *
     * @param service
     */
    explicit context(service_info service) noexcept : _service(std::move(service)) {}

    /**
     * If context is created without service information,
     */
    context() noexcept = default;

    /**
     * Call RPC function. Will be load-balanced automatically.
     *
     * @tparam RetVal_
     * @tparam Params_
     * @param params
     * @return
     */
    template <typename RetVal_, typename... Params_>
    auto rpc(std::string_view method, Params_&&... params) -> std::future<RetVal_>;

    /**
     * Notify single session
     */
    template <typename... Params_>
    void notify(std::string_view method, Params_&&... params);

    /**
     * Notify all sessions
     */
    template <typename... Params_>
    void notify_all(std::string_view method, Params_&&... params);

    /**
     * Create new session with given connection type.
     * Lifecycle of connection must be managed outside of class boundary.
     *
     * @tparam Conn_
     * @tparam Args_
     * @param args
     * @return shared reference to
     */
    template <typename Conn_,
              typename... Args_,
              typename = std::enable_if_t<std::is_base_of_v<std::streambuf, Conn_>>>
    [[nodiscard]] auto
    create_session(Args_&&... args) -> std::shared_ptr<Conn_>;
};

}  // namespace CPPHEADERS_NS_::msgpack::rpc
