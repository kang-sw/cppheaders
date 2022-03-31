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
class if_connection_streambuf : public std::streambuf
{
   private:
    friend class session;
    if_session* _owner;

   public:
    /**
     * Initialize this connection. Must not throw.
     */
    virtual void initialize() noexcept = 0;

    /**
     * start waiting for new data
     */
    virtual void async_wait_data() noexcept = 0;

    /**
     * Close this session
     */
    virtual void close() noexcept = 0;

    /**
     * Get total number of read/write bytes
     */
    virtual void get_total_rw(size_t* num_read, size_t* num_write) = 0;

   protected:
    void on_data_in() noexcept
    {
        _owner->on_data_wait_complete();
    }
};

}  // namespace CPPHEADERS_NS_::rpc