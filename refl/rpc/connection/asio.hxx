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
#include "../../__namespace__"
#include "../detail/connection.hxx"
#include "../detail/interface.hxx"
#include "asio/basic_socket_streambuf.hpp"
#include "asio/post.hpp"

namespace CPPHEADERS_NS_::rpc {
class _asio_count_adapter_streambuf : public std::streambuf
{
   public:
    std::streambuf* _owner;
    size_t          _rt = 0, _wt = 0;

   public:
    explicit _asio_count_adapter_streambuf(std::streambuf* owner) : _owner(owner) {}

   protected:
    int sync() override
    {
        return _owner->pubsync();
    }

    std::streamsize xsgetn(char_type* buf, std::streamsize n) override
    {
        auto nread = _owner->sgetn(buf, n);
        _rt += nread;
        return nread;
    }

    int underflow() override
    {
        return _owner->sgetc();
    }

    int uflow() override
    {
        _rt += 1;
        return _owner->sbumpc();
    }

    std::streamsize xsputn(const char_type* buf, std::streamsize n) override
    {
        auto nwrite = _owner->sputn(buf, n);
        _wt += nwrite;
        return nwrite;
    }

    int overflow(int_type c) override
    {
        _wt += 1;
        return _owner->sputc(c);
    }
};

template <typename Protocol>
class asio_stream : public if_connection
{
    using protocol_type = Protocol;
    using socket_type = typename Protocol::socket;
    using streambuf_type = asio::basic_socket_streambuf<protocol_type>;

   private:
    streambuf_type                _buf;
    _asio_count_adapter_streambuf _adapter{&_buf};

   public:
    explicit asio_stream(socket_type&& socket)
            : if_connection(&_adapter, _ep_to_string(socket)),
              _buf(std::move(socket))
    {
    }

   public:
    void start_data_receive() noexcept override
    {
        if (_buf.in_avail())
            on_data_receive();
        else
            _buf.socket().async_wait(
                    asio::socket_base::wait_read,
                    [this](auto&& ec) {
                        if (ec) { return; }
                        on_data_receive();
                    });
    }

    void close() noexcept override
    {
        _buf.close();
    }

    void get_total_rw(size_t* num_read, size_t* num_write) override
    {
        *num_read = _adapter._rt;
        *num_write = _adapter._wt;
    }

   private:
    static std::string _ep_to_string(socket_type const& socket)
    {
        auto ep = socket.remote_endpoint();
        return ep.address().to_string() + ":" + std::to_string(ep.port());
    }
};

template <typename StreamSock>
asio_stream(StreamSock&& sock) -> asio_stream<typename StreamSock::protocol_type>;

template <class Executor>
class asio_event_procedure : public if_event_proc
{
    std::unique_ptr<Executor> _ref_autorelease;
    Executor*                 _ref;

   public:
    explicit asio_event_procedure(Executor& exec) : _ref(&exec) {}
    explicit asio_event_procedure(std::unique_ptr<Executor> exec)
            : _ref_autorelease(std::move(exec)),
              _ref(_ref_autorelease.get()) {}

   public:
    auto executor() const noexcept { return _ref; }

   public:
    void post_rpc_completion(function<void()>&& fn) override
    {
        asio::post(*_ref, std::move(fn));
    }

    void post_handler_callback(function<void()>&& fn) override
    {
        asio::post(*_ref, std::move(fn));
    }

    void post_internal_message(function<void()>&& fn) override
    {
        asio::post(*_ref, std::move(fn));
    }
};

template <class Executor>
asio_event_procedure(Executor&) -> asio_event_procedure<Executor>;

template <class Executor>
asio_event_procedure(std::unique_ptr<Executor>) -> asio_event_procedure<Executor>;

inline auto asio_global_event_procedure()
{
    class procedure_t : public if_event_proc
    {
       public:
        void post_rpc_completion(function<void()>&& fn) override
        {
            asio::post(std::move(fn));
        }

        void post_handler_callback(function<void()>&& fn) override
        {
            asio::post(std::move(fn));
        }

        void post_internal_message(function<void()>&& fn) override
        {
            asio::post(std::move(fn));
        }
    };

    static auto _procedure = std::make_shared<procedure_t>();
    return _procedure;
}

}  // namespace CPPHEADERS_NS_::rpc
