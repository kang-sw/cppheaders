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

#include "../detail/connection.hxx"
#include "../detail/interface.hxx"
#include "asio/basic_socket_streambuf.hpp"
#include "asio/post.hpp"

namespace cpph::rpc {
template <typename Protocol>
class asio_stream : public if_connection, public std::streambuf
{
    using protocol_type = Protocol;
    using socket_type = typename Protocol::socket;
    using streambuf_type = asio::basic_socket_streambuf<protocol_type>;

   private:
    socket_type _socket;

    char _wrbuf[1500];
    char _rdbuf[1500];

    size_t _total_read = 0;
    size_t _total_write = 0;

    bool _closed = false;

   public:
    explicit asio_stream(socket_type&& socket)
            : if_connection(this, _ep_to_string(socket)),
              _socket(std::move(socket))
    {
        _socket.set_option(asio::ip::tcp::no_delay{true});
    }

   public:
    void start_data_receive() noexcept override
    {
        if (in_avail()) {
            on_data_receive();
        } else {
            _socket.async_read_some(
                    asio::null_buffers{},
                    [this](auto&& ec, auto) {
                        if (ec) { return; }
                        on_data_receive();
                    });
        }
    }

    void close() noexcept override
    {
        _socket.close();
    }

    void get_total_rw(size_t* num_read, size_t* num_write) override
    {
        *num_read = _total_read;
        *num_write = _total_write;
    }

   protected:
    int_type overflow(int_type c) override
    {
        if (_write_all() != ~size_t{}) {
            _wrbuf[0] = traits_type::to_char_type(c);
            pbump(1);

            return c;
        } else {
            return traits_type::eof();
        }
    }

    int_type underflow() override
    {
        asio::error_code ec;

        auto nread = _socket.receive(asio::buffer(_rdbuf), {}, ec);
        _total_read += nread;
        setg(_rdbuf, _rdbuf, _rdbuf + nread);

        if (ec) {
            return traits_type::eof();
        } else {
            return traits_type::to_int_type(_rdbuf[0]);
        }
    }

    std::streamsize xsgetn(char* buf, std::streamsize len) override
    {
        if (_closed) { return 0; }

        if (len < sizeof _rdbuf) {
            return basic_streambuf::xsgetn(buf, len);
        } else {
            // Read from cache first
            std::streamsize nread = egptr() - gptr();
            memcpy(buf, gptr(), nread);

            setg(nullptr, nullptr, nullptr);
            asio::error_code ec;

            do {
                nread += _socket.receive(asio::buffer(buf + nread, len - nread), {}, ec);
            } while (not ec && nread < len);

            _total_read += nread;
            return nread;
        }
    }

    std::streamsize xsputn(const char* data, std::streamsize len) override
    {
        if (_closed) { return 0; }

        if (len < sizeof _wrbuf) {
            return basic_streambuf::xsputn(data, len);
        } else {
            _write_all();  // First, flush current buffer

            std::streamsize nwrite = 0;
            asio::error_code ec;

            do {
                auto n = _socket.send(asio::buffer(data, len - nwrite), {}, ec);
                data += n, nwrite += n;
            } while (not ec && nwrite < len);

            _total_write += nwrite;
            return nwrite;
        }
    }

    int sync() override
    {
        if (~size_t{} == _write_all()) {
            return -1;
        }

#if defined(ASIO_IP_TCP_HPP)  // Only when TCP header is included
        try {
            //! Force flushing data
            _socket.set_option(asio::ip::tcp::no_delay{true});
            _socket.set_option(asio::ip::tcp::no_delay{false});
        } catch (asio::system_error&) {
            return -1;
        }
#endif
        return basic_streambuf::sync();
    }

   private:
    size_t _write_all()
    {
        asio::error_code ec;
        auto num_send = pptr() - pbase();

        while (num_send > 0) {
            auto n = _socket.send(asio::buffer(_wrbuf, num_send), {}, ec);
            num_send -= n, _total_write += n;

            if (ec) {
                _closed = true;
                return ~size_t{};
            }
        }

        setp(_wrbuf, _wrbuf + sizeof _wrbuf);
        return num_send;
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
    Executor* _ref;

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

}  // namespace cpph::rpc
