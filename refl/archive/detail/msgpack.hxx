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
#include "../../../__namespace__"

namespace CPPHEADERS_NS_::archive::msgpack {
enum class typecode : uint8_t {
    positive_fixint = 0b0'0000000,
    negative_fixint = 0b111'00000,

    fixmap = 0b1000'0000,
    fixarray = 0b1001'0000,
    fixstr = 0b101'00000,

    error = 0xc1,

    nil = 0xc0,
    bool_false = 0xc2,
    bool_true = 0xc3,

    bin8 = 0xc4,
    bin16,
    bin32,

    float32 = 0xca,
    float64 = 0xcb,
    uint8 = 0xcc,
    uint16 = 0xcd,
    uint32 = 0xce,
    uint64 = 0xcf,
    int8 = 0xd0,
    int16 = 0xd1,
    int32 = 0xd2,
    int64 = 0xd3,

    //! @warning EXT types are treated as simple binary. Typecodes will be ignored.
    fixext1 = 0xd4,
    fixext2,
    fixext4,
    fixext8,
    fixext16,

    ext8 = 0xc7,
    ext16,
    ext32,

    str8 = 0xd9,
    str16 = 0xda,
    str32 = 0xdb,

    array16 = 0xdc,
    array32 = 0xdd,

    map16 = 0xde,
    map32 = 0xdf,
};
}
