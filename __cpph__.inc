// MIT License
//
// Copyright (c) 2022. Seungwoo Kang
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
 * Configurations
 */

//! Stacktrace library ... default disabled
//! Since enabling this library creates a dedicated watcher thread and installs
//!  custom abort signal handler, enabling backward library for multiple
//!  instance is redundant and may affect performance badly.
//! Thus, enable this library only once over compilation units in executable is recommended.
#ifndef CPPHEADERS_ENABLE_BACKWARD
#    define CPPHEADERS_ENABLE_BACKWARD 0
#endif

/**
 * Required sources for compilation
 *
 * DO NOT INCLUDE THIS FILE MORE THAN ONCE!
 */
#include "refl/archivers/__cpph__.inc"

// DO NOT CHANGE THE ORDER!
// ASSERTION MUST BE PLACED LAST
#define CPPHEADERS_IMPLEMENT_ASSERTIONS
#include "assert.hxx"
