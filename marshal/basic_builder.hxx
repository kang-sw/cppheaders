
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

/**
 * usage:
 * \code
 *   std::string buffer;
 *
 *   // forward args for construct json_serializer
 *   basic_builder<json_serializer> obj(&buffer, ...);
 *
 *   // with simple assignment, it'll replace current context.
 *   // (key-index context/ value context)
 *   obj = 4;              // 4
 *   // obj = "hell, world"; // error: invalid context
 *
 *   obj.clear();          //
 *   obj = "hell, world!"; // "hell, world!"
 *
 *   obj.clear();
 *
 *   // with index operator, it creates new objects
 *   // since current context is not object, it'll reset.
 *   obj["a"];     // {"a":
 *   obj["b"];     // {"a": {"b":
 *   obj = 3;      // {"a": {"b": 3
 *
 *   obj["c"];     // {"a": {"b": 3, "c":
 *   obj = "cc";   // {"a": {"b": 3, "c": "cc"
 *
 *   obj.break();  // {"a": {"b": 3, "c": "cc"}
 *
 *   obj["d"];     // {"a": {"b": 3, "c": "cc"}, "d":
 *
 *   // obj.break(); // error: invalid builder context

 *   obj = nullptr;  // {"a": {"b": 3, "c": "cc"}, "d": null
 *   obj.break();    // {"a": {"b": 3, "c": "cc"}, "d": null}
 *
 *   obj.clear();
 *
 *   obj[0];         // [
 *   obj = 1;        // [1
 *   obj[2];         // [1, null,
 *
 *   // obj[1]; // error: cannot invert index order
 *   obj = "ola";    // [1, null, "ola"
 *   obj["brk"];     // [1, null, "ola", {"brk":
 *   obj = nullptr;  // [1, null, "ola", {"brk": null
 *
 *   obj.break().break(); // [1, null, "ola", {"brk": null}]
 * \endcode
 */