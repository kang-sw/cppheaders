
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
#include <cstring>
#include <exception>

//
#include "../__namespace__"

namespace CPPHEADERS_NS_ {

template <typename Ty_>
struct basic_exception : std::exception {
   private:
    mutable std::string _message = {};

   public:
    void message(std::string_view content)
    {
        _setmsg(content);
    }

    template <typename Str_, typename Arg0_, typename... Args_>
    void message(Str_&& str, Arg0_&& arg0, Args_&&... args)
    {
        auto buflen = snprintf(nullptr, 0, str, arg0, args...);
        _message.resize(buflen + 1);

        snprintf(_message.data(), _message.size(), str, arg0, args...);
        _message.pop_back();  // remove null character
    }

    /**
     * For inheritances ...
     */
    template <typename RTy_>
    RTy_&& as()
    {
        assert(dynamic_cast<RTy_*>(this));
        return std::move(*static_cast<RTy_*>(this));
    }

    const char* what() const noexcept override
    {
        if (_message.empty()) { _setmsg(typeid(*this).name()); }
        return _message.c_str();
    }

   private:
    void _setmsg(std::string_view content) const
    {
        if (_message.empty()) {
            _message = "error (";
            _message += typeid(*this).name();
            _message += "): ";
        }
        _message += content;
    }
};

}  // namespace CPPHEADERS_NS_

#ifndef CPPH_DECLARE_EXCEPTION
#    define CPPH_DECLARE_EXCEPTION(Name, Base)                \
        struct Name : Base {                                  \
            using _cpph_internal_super = Base;                \
            using _cpph_internal_super::_cpph_internal_super; \
        }
#endif
