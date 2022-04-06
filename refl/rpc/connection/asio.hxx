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
#include "../detail/interface.hxx"
#include "asio/basic_socket_streambuf.hpp"

namespace CPPHEADERS_NS_::rpc::connection {

template <typename StreamSock>
class asio_stream : public if_connection
{
    using protocol_type = typename StreamSock::protocol_type;
    using streambuf_type = asio::basic_socket_streambuf<protocol_type>;

   private:
    streambuf_type _buf;
    size_t         _rt = 0, _wt = 0;

   public:
    explicit asio_stream(StreamSock&& socket)
            : if_connection(&_buf, _ep_to_string(socket)),
              _buf(std::move(socket))
    {
    }

   public:
    void initialize() noexcept override
    {
    }

    void start_data_receive() noexcept override
    {
    }

    void close() noexcept override
    {
    }

    void get_total_rw(size_t* num_read, size_t* num_write) override
    {
    }

   private:
    static std::string _ep_to_string(StreamSock const& socket)
    {
        auto ep = socket.remote_endpoint();
        return ep.address().to_string() + ":" + std::to_string(ep.port());
    }
};

template <typename StreamSock>
asio_stream(StreamSock&& sock) -> asio_stream<StreamSock>;

}  // namespace CPPHEADERS_NS_::rpc::connection
