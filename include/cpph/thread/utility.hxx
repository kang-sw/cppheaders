
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
#include <mutex>

//

namespace cpph {

template <typename Pointer_>
void wait_pointer_unique(Pointer_&& ptr)
{
    if (not ptr) { return; }
    while (ptr.use_count() != 1)
        std::this_thread::yield();
}

template <typename Pointer_, typename Dur_>
bool wait_pointer_unique(Pointer_&& ptr, Dur_&& duration)
{
    auto now = [] { return std::chrono::steady_clock::now(); };
    auto until = now() + duration;

    if (not ptr) { return true; }

    do {
        if (ptr.use_count() == 1)
            return true;

        std::this_thread::yield();
    } while (now() < until);

    return false;
}

}  // namespace cpph