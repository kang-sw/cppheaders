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
#include "../../../circular_queue.hxx"
#include "../../../spinlock.hxx"
#include "../../__namespace__"
#include "../connection.hxx"

namespace CPPHEADERS_NS_::rpc::connection {
class inmemory_pipe : public if_connection_streambuf
{
    struct pipe {
        spinlock             lock;
        circular_queue<char> strm{1024};

        inmemory_pipe*       receiver;
    };

   private:
    int              _idx = 0;
    shared_ptr<pipe> _in, _out;

    char             _ibuf[2048];
    char             _obuf[2048];

   private:
    inmemory_pipe() : if_connection_streambuf("INMEMORY" + std::to_string((intptr_t)this)) {}

   public:
    static auto create()
    {
        unique_ptr<inmemory_pipe> insts[2];
        shared_ptr<pipe>          pipes[2];

        for (auto& p : insts) { p.reset(new inmemory_pipe); }
        for (auto& p : pipes) { p = std::make_shared<pipe>(); }

        {
            auto& [ia, ib] = insts;
            auto& [pa, pb] = pipes;
            ia->_in = ib->_out = pa;
            ia->_out = ib->_in = pb;

            pa->receiver = &*ia;
            pb->receiver = &*ib;
        }
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

   protected:
    int_type overflow(int_type int_type) override
    {
        return basic_streambuf::overflow(int_type);
    }
    int_type underflow() override
    {
        return basic_streambuf::underflow();
    }
};

}  // namespace CPPHEADERS_NS_::rpc::connection
