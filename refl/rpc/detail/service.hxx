
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
#include <map>

#include "../../__namespace__"
#include "defs.hxx"
#include "interface.hxx"
#include "signature.hxx"

namespace CPPHEADERS_NS_::rpc {
using std::enable_if_t, std::is_convertible_v;
using std::move, std::forward;
using std::string, std::string_view, std::tuple;

using service_table_t
        = std::map<string, unique_ptr<if_service_handler>, std::less<>>;

class service
{
    friend class service_builder;
    shared_ptr<service_table_t> _service;

   public:
    shared_ptr<if_service_handler>
    find_handler(string_view method_name) const noexcept
    {
        if (auto iter = _service->find(method_name); iter != _service->end())
            return shared_ptr<if_service_handler>{_service, iter->second.get()};
        else
            return nullptr;
    }
};
}  // namespace CPPHEADERS_NS_::rpc