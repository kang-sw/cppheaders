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
#include <memory>

#include "../__namespace__"

namespace CPPHEADERS_NS_::msgpack::rpc {
class context;

namespace detail {
class session;
}

enum class rpc_status {
    okay                = 0,
    waiting             = 1,

    aborted             = -20,
    timeout             = -10,

    unknown_error       = -1,
    internal_error      = -2,
    invalid_parameter   = -3,
    invalid_return_type = -4,

    method_not_exist    = -5,

    dead_peer           = -100,
};

enum class rpc_type {
    request = 0,
    reply   = 1,
    notify  = 2,
};

namespace async_rpc_result {
enum type : int {
    invalid              = 0,
    error                = -1,

    no_active_connection = -10,
    invalid_parameters   = -11,
    invalid_connection   = -12,
};
}
}  // namespace CPPHEADERS_NS_::msgpack::rpc
