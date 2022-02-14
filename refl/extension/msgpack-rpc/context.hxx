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
#include <map>

#include "../../../functional.hxx"
#include "../../../helper/exception.hxx"
#include "../../__namespace__"
#include "../../archive/msgpack-reader.hxx"
#include "../../archive/msgpack-writer.hxx"
#include "../../detail/object_core.hxx"

namespace CPPHEADERS_NS_::msgpack::rpc {

CPPH_DECLARE_EXCEPTION(exception, std::exception);
CPPH_DECLARE_EXCEPTION(invalid_connection, exception);

class if_connection : public std::enable_shared_from_this<if_connection>
{
   public:  // signaling
    //! on received data from buffer ...
    virtual void on_read(array_view<void const> payload) = 0;

   public:  // public interface
    //! returns human-readable peer name.
    virtual std::string peer() const = 0;

    //! may block forever until all buffer content processed
    //! @throw invalid_connection when connection became invalid state
    virtual void write(array_view<void const> payload) = 0;
};

namespace detail {
class session
{
    std::shared_ptr<if_connection> _conn;
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
    void
    serve2(Str_&& method_name, function<void(RetVal_*, Params_...)> handler);

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
    void serve(Str_&& method_name, function<RetVal_(Params_...)> handler)
    {
        serve2(std::forward<Str_>(method_name),
               [_handler = std::move(handler)]  //
               (RetVal_ * buffer, Params_ && ... args) mutable {
                   *buffer = _handler(std::forward<Params_>(args)...);
               });
    }
};

class context
{
    service_info _service;

   public:
    /**
     * Create new context with given service information.
     * Once service is registered, it becomes read-only.
     *
     * @param service
     */
    context(service_info&& service) noexcept;

    /**
     * Call RPC function.
     *
     * @tparam RetVal_
     * @tparam Params_
     * @param params
     * @return
     */
    template <typename RetVal_, typename... Params_>
    auto rpc(std::string_view method, Params_&&... params) -> std::future<RetVal_>;

    /**
     * Call notify
     */
    template <typename... Params_>
    void notify(std::string_view method, Params_&&... params);

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
              typename = std::enable_if_t<std::is_base_of_v<if_connection, Conn_>>>
    [[nodiscard]] auto
    create_session(Args_&&... args) -> std::shared_ptr<Conn_>;
};

}  // namespace CPPHEADERS_NS_::msgpack::rpc
