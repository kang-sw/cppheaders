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
#include "../../futils.hxx"
#include "../detail/if_archive.hxx"

namespace CPPHEADERS_NS_::archive {

class debug_string_writer : public if_writer
{
    enum class context_state
    {
        empty,
        object_key,
        object_value,
        array
    };

   private:
    int _idgen           = 0;
    bool _comma_required = false;

    std::vector<int> _id_stack;
    std::vector<context_state> _state_stack;

   private:
    void write_str(std::string_view str)
    {
        _buf->sputn(str.data(), str.size());
    }

    template <typename... Args_>
    void _write_f(char const* fmt, Args_... args)
    {
        auto str = futils::usprintf(fmt, args...);
        _buf->sputn(str, strlen(str));
    }

    template <typename... Args_>
    auto& _write_value(char const* fmt, Args_... args)
    {
        _pre_write();
        _write_f(fmt, args...);
        _post_write();
        return *this;
    }

    int _nlevel() const
    {
        return int(_id_stack.size());
    }

    void _pre_write()
    {
        switch (_state())
        {
            case context_state::empty:
                throw error::writer_invalid_state(this).message("write when empty!");

            case context_state::object_key:
                if (_comma_required)
                    write_str(",\n");
                else
                    write_str("\n");

                _indent();
                return;

            case context_state::object_value:
                break;

            case context_state::array:
                if (_comma_required)
                    write_str(",\n");
                else
                    write_str("\n");

                _indent();
                break;
        }
    }

    void _post_write()
    {
        switch (_state())
        {
            case context_state::object_key:
                _state(context_state::object_value);
                write_str(": ");
                break;

            case context_state::object_value:
                _state(context_state::object_key);
                _comma_required = true;
                break;

            case context_state::array:
                _comma_required = true;
                break;

            default:
                assert(false && "Must not enter here");
        }
    }

    void _indent()
    {
        std::string buf;
        if (_nlevel() > 0)
            buf.resize(_nlevel() * 2, ' ');

        write_str(buf);
    }

    context_state _state() const noexcept
    {
        return _state_stack.empty() ? context_state::empty : _state_stack.back();
    }

    void _state(context_state v)
    {
        _state_stack.back() = v;
    }

   public:
    explicit debug_string_writer(std::streambuf& buf) : if_writer(buf)
    {
    }

    if_writer& operator<<(bool v) override
    {
        return _write_value(v ? "true" : "false");
    }

    if_writer& operator<<(nullptr_t a_nullptr) override
    {
        return _write_value("null");
    }

    if_writer& operator<<(int64_t v) override
    {
        return _write_value("%lld", v);
    }

    if_writer& operator<<(double v) override
    {
        return _write_value("%f", v);
    }

    if_writer& operator<<(std::string_view v) override
    {
        return _write_value("%s", std::string(v).c_str());
    }

    if_writer& binary_write_some(const_buffer_view v) override
    {
        char charset[] = "0123456789ABCDEF";

        for (auto ch : v)
        {
            _buf->sputc('x');
            _buf->sputc(charset[(ch & 0xf0) >> 4]);
            _buf->sputc(charset[(ch & 0x0f) >> 0]);
            _buf->sputc(' ');
        }

        return *this;
    }

    if_writer& binary_push(size_t total) override
    {
        return *this;
    }
    if_writer& binary_pop() override
    {
        _write_value("");
        return *this;
    }

    if_writer& object_push(size_t n) override
    {
        if (_state() == context_state::object_key)
            throw error::writer_invalid_state{this}.message("invalid state: object_key");

        if (_state() == context_state::object_value)
            _state(context_state::object_key);

        if (_comma_required && _state() == context_state::array)
            write_str(",\n"), _indent();

        write_str("{");

        _comma_required = false;
        _id_stack.push_back(++_idgen);
        _state_stack.push_back(context_state::object_key);
        return *this;
    }

    if_writer& object_pop() override
    {
        if (_state() != context_state::object_key)
            throw error::writer_invalid_state{this}.message("expect object_key");

        bool was_empty  = not _comma_required;
        _comma_required = true;

        _id_stack.pop_back();
        _state_stack.pop_back();

        if (not was_empty)
        {
            write_str("\n");
            _indent();
        }

        write_str("}");

        return *this;
    }

    if_writer& array_push(size_t n) override
    {
        if (_state() == context_state::object_key)
            throw error::writer_invalid_state{this}.message("invalid state: object_key");

        if (_state() == context_state::object_value)
            _state(context_state::object_key);

        if (_comma_required && _state() == context_state::array)
            write_str(",\n"), _indent();

        write_str("[");

        _comma_required = false;
        _id_stack.push_back(++_idgen);
        _state_stack.push_back(context_state::array);
        return *this;
    }

    if_writer& array_pop() override
    {
        if (_state() != context_state::array)
            throw error::writer_invalid_state{this}.message("expect array");

        bool was_empty  = not _comma_required;
        _comma_required = true;

        _id_stack.pop_back();
        _state_stack.pop_back();

        if (not was_empty)
        {
            write_str("\n");
            _indent();
        }

        write_str("]");

        return *this;
    }

    void write_key_next() override
    {
        if (_state() != context_state::object_key)
            throw error::writer_invalid_state{this}.message("");
    }
};

}  // namespace CPPHEADERS_NS_::archive
