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
#include <memory>
#include <streambuf>

#include "../../__namespace__"
#include "defs.hxx"
#include "interface.hxx"

namespace CPPHEADERS_NS_::rpc {
class if_connection
{
   private:
    friend class session;
    std::weak_ptr<if_session> _wowner;
    std::streambuf* const     _buf;

   public:
    std::string const peer_name;

   public:
    virtual ~if_connection() noexcept = default;

    /**
     *
     */
    explicit if_connection(std::streambuf* buf, std::string peer_name) noexcept
            : _buf(buf),
              peer_name(std::move(peer_name)) {}

    /**
     * Get streambuf
     */
    auto* streambuf() const noexcept { return _buf; }

    /**
     * Initialize this connection. Must not throw.
     */
    virtual void initialize() noexcept = 0;

    /**
     * Start waiting for new data
     *
     * It may call on_data_receive() immediately if there's any data ready, however,
     *  it must not block for waiting incoming data.
     */
    virtual void start_data_receive() noexcept = 0;

    /**
     * Close this session
     */
    virtual void close() noexcept = 0;

    /**
     * Get total number of read/write bytes
     */
    virtual void get_total_rw(size_t* num_read, size_t* num_write) = 0;

   protected:
    void on_data_receive() noexcept
    {
        if (auto owner = _wowner.lock())
            owner->on_data_wait_complete();
    }
};

}  // namespace CPPHEADERS_NS_::rpc
