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
#include <cassert>
#include <memory>

#include "../../__namespace__"
#include "defs.hxx"

namespace CPPHEADERS_NS_::rpc {

class request_handle
{
    friend class session;

    std::weak_ptr<session> _wp;

    union {
        int _msgid = 0;
        int _error;
    };

   public:
    operator bool() const noexcept { return _msgid > 0 && not _wp.expired(); }

    //
    auto errc() const noexcept
    {
        return request_result{_msgid < 0 ? -_msgid : 0};
    }

    template <typename Duration_, typename Sptr_ = std::shared_ptr<session>>
    bool wait(Duration_&& duration) const noexcept
    {
        assert(_msgid > 0);

        if (Sptr_ session = _wp.lock())
            return session->wait_rpc(_msgid, duration);
        else
            return false;
    }

    template <typename Sptr_ = std::shared_ptr<session>>
    bool abort() const noexcept
    {
        assert(_msgid > 0);

        if (Sptr_ session = _wp.lock())
            return session->abort_rpc(_msgid);
        else
            return false;
    }

    void reset()
    {
        _msgid = 0;
        _wp = {};
    }
};
}  // namespace CPPHEADERS_NS_::rpc
