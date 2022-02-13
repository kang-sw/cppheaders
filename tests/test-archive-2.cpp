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

// H
#include <iostream>
#include <list>
#include <optional>
#include <sstream>

#include "refl/object_core.hxx"

struct base_object
{
    std::optional<double> opt_double;
    std::list<int> list_int;
    cpph::binary<std::vector<char>> bin_vec_chars;

    void fill()
    {
        opt_double = 1.;
        list_int   = {1, 2, 34};
        bin_vec_chars.assign({'h', 'e', 'l', 'l', 'o'});
    }
};

CPPH_REFL_DECLARE(base_object);

// CPP

#include "catch.hpp"
#include "refl/archive/debug_string_writer.hxx"
#include "refl/archive/json.hpp"
#include "refl/archive/msgpack-reader.hxx"
#include "refl/archive/msgpack-writer.hxx"
#include "refl/buffer.hxx"
#include "refl/container/variant.hxx"
#include "refl/object.hxx"
