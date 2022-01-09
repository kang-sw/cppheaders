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

#pragma once
#include "../../futils.hxx"
#include "../if_archive.hxx"

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
    template <typename... Args_>
    void _write_f(char const* fmt, Args_... args)
    {
        write(futils::ssprintf(fmt, args...));
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
    explicit debug_string_writer(stream_writer ostrm) : if_writer(std::move(ostrm))
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

    if_writer& operator<<(const binary_t& v) override
    {
        return _write_value("binary [%lld] bytes", v.size());
    }

    if_writer& write_binary(array_view<const char> v) override
    {
        return *this;
    }

    if_writer& object_push() override
    {
        if (_state() == context_state::object_key)
            throw error::writer_invalid_context{this}.message("invalid state: object_key");

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
            throw error::writer_invalid_context{this}.message("expect object_key");

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

    if_writer& array_push() override
    {
        if (_state() == context_state::object_key)
            throw error::writer_invalid_context{this}.message("invalid state: object_key");

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
            throw error::writer_invalid_context{this}.message("expect array");

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

    bool is_key_next() const override
    {
        return _state() == context_state::object_key;
    }
};

}  // namespace CPPHEADERS_NS_::archive
