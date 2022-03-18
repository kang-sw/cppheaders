// MIT License
//
// Copyright (c) 2021-2022. Seungwoo Kang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// project home: https://github.com/perfkitpp

#pragma once
#include <asio/basic_socket.hpp>
#include <asio/basic_socket_acceptor.hpp>
#include <asio/basic_socket_streambuf.hpp>
#include <asio/bind_executor.hpp>
#include <asio/dispatch.hpp>
#include <asio/ip/basic_endpoint.hpp>
#include <asio/strand.hpp>

#include "../__namespace__"
#include "context.hxx"

namespace CPPHEADERS_NS_::msgpack::rpc::asio_ex {

namespace detail {
template <typename Protocol_>
class transient_socket_streambuf : public std::streambuf
{
   public:
    using socket_type = typename Protocol_::socket;

   private:
    socket_type               _socket;
    std::chrono::microseconds _timeout = {};

    char                      _wbuf[2048];
    char                      _rbuf[2048];

    size_t                    _nread  = 0;
    size_t                    _nwrite = 0;

   public:
    explicit transient_socket_streambuf(socket_type socket)
            : _socket(std::move(socket))
    {
        setp(_wbuf, *(&_wbuf + 1));
        setg(_rbuf, _rbuf, _rbuf);
    }

    auto& socket() { return _socket; }

   protected:
    int_type overflow(int_type type) override
    {
        try {
            auto nready = pptr() - pbase();
            _nwrite += nready;

            while (nready > 0) {
                nready -= _socket.send(asio::buffer(pbase(), nready));
            }

            setp(_wbuf, *(&_wbuf + 1));

            if (traits_type::not_eof(type))
                return sputc(traits_type::to_char_type(type));
            else
                return 0;
        } catch (asio::system_error& ec) {
            return traits_type::eof();
        }
    }

    int_type underflow() override
    {
        try {
            auto navail = _socket.receive(asio::buffer(_rbuf));
            _nread += navail;

            setg(_rbuf, _rbuf, _rbuf + navail);

            return traits_type::to_int_type(_rbuf[0]);
        } catch (asio::system_error& ec) {
            return traits_type::eof();
        }
    }

    int sync() override
    {
        return overflow(traits_type::eof());
    }

    std::streamsize showmanyc() override
    {
        return _showmanyc_impl();
    }

    size_t _showmanyc_impl()
    {
        try {
            asio::socket_base::bytes_readable navail{true};
            _socket.io_control(navail);
            return navail.get();
        } catch (asio::system_error&) {
            throw invalid_connection{};
        }
    }

   public:
    void set_timeout(std::chrono::microseconds microseconds)
    {
        _timeout = microseconds;
    }

    void nrw(size_t* nwrite, size_t* nread) const noexcept
    {
        *nwrite = _nwrite;
        *nread  = _nread;
    }
};

template <typename Protocol_>
class basic_socket_connection : public if_connection
{
   public:
    using socket        = typename Protocol_::socket;
    using streambuf     = asio::basic_socket_streambuf<Protocol_>;
    using socket_buffer = transient_socket_streambuf<Protocol_>;

   private:
    socket_buffer   _buf;
    std::atomic_int _wait_counter = 0;

   public:
    explicit basic_socket_connection(socket sock)
            : if_connection(peer_string(sock.remote_endpoint())),
              _buf(std::move(sock))
    {
    }

   protected:
    //! Gets underlying socket reference, for
    socket& ref_socket() noexcept { return _buf.socket(); }

   public:
    std::streambuf* rdbuf() override
    {
        return &_buf;
    }

    void begin_wait() override
    {
        auto fn = bind_front_weak(
                owner(),
                [this](asio::error_code const& ec) {
                    if (ec) { this->notify_disconnect(); }

                    this->notify_receive();
                });

        if (_buf.in_avail())
            this->notify_receive();
        else
            _buf.socket().async_wait(socket::wait_read, std::move(fn));
    }

    void launch() override
    {
        ;  // There's nothing to do explicitly on launch.
    }

    void disconnect() override
    {
        _buf.socket().close();
    }

    void set_timeout(std::chrono::microseconds microseconds) override
    {
        _buf.set_timeout(microseconds);
    }

    void totals(size_t* nwrite, size_t* nread) const override
    {
        _buf.nrw(nwrite, nread);
    }

   private:
    static std::string peer_string(typename socket::endpoint_type const& ep)
    {
        std::string buf;
        buf.reserve(128);

        buf += ep.address().to_string();
        buf += ':';
        buf += std::to_string(ep.port());

        return buf;
    }
};
}  // namespace detail

/**
 * Start acceptor instance
 *
 * Acceptor should be bound to address before passed to this function.
 */
template <typename Protocol_, typename Exec_>
auto open_acceptor(
        context&                                       ctx,
        session_config const&                          configs,
        asio::basic_socket_acceptor<Protocol_, Exec_>& acceptor,
        asio::strand<Exec_>*                           pstrand = nullptr)
{
    using strand_type   = asio::strand<Exec_>;
    using acceptor_type = std::remove_reference_t<decltype(acceptor)>;
    using socket_type   = typename Protocol_::socket;

    struct _accept_function
    {
        acceptor_type* _acceptor;
        context*       _ctx;

        std::shared_ptr<std::tuple<socket_type,
                                   asio::strand<Exec_>,
                                   session_config>>
                _body;

        explicit _accept_function(context& ctx, session_config const& cfg, acceptor_type& acpt, strand_type strand2)
                : _acceptor(&acpt),
                  _ctx(&ctx),
                  _body(std::make_shared<typename decltype(_body)::element_type>(
                          socket_type(acpt.get_executor()),
                          std::move(strand2),
                          cfg))
        {
        }

        void operator()(asio::error_code ec)
        {
            if (ec) { return; }  // propagate error to executor

            auto& [_sock, _strand, _cfg] = *_body;

            // Register accepted socket as context client
            _ctx->create_session<detail::basic_socket_connection<Protocol_>>(_cfg, std::move(_sock));

            //
            (std::move(*this)).async_accept();
        }

        void async_accept()
        {
            auto& [_sock, _strand, _cfg] = *_body;

            // Prepare for next accept
            _acceptor->async_accept(
                    _sock,
                    asio::bind_executor(
                            _strand,
                            *this));
        }
    };

    acceptor.listen();

    auto strand = pstrand ? *pstrand : strand_type{acceptor.get_executor()};
    _accept_function{ctx, configs, acceptor, std::move(strand)}.async_accept();
}

/**
 * Create session from existing socket.
 *
 * @return
 */
template <typename Socket_,
          typename = std::enable_if_t<std::is_base_of_v<asio::socket_base, Socket_>>>
auto create_session(context& rpc, Socket_&& socket, session_config const& config = {})
{
    using connection_type = detail::basic_socket_connection<typename Socket_::protocol_type>;
    return rpc.create_session<connection_type>(
            config,
            std::forward<Socket_>(socket));
}

}  // namespace CPPHEADERS_NS_::msgpack::rpc::asio_ex