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

#if __has_include(<nlohmann/json.hpp>)
#    include <list>

#    include <nlohmann/json.hpp>

#    include "cpph/refl/detail/primitives.hxx"

namespace cpph::refl::_detail {

inline void _archive_recursive(archive::if_writer* strm, const nlohmann::json& data)
{
    switch (data.type()) {
        case nlohmann::detail::value_t::null: strm->write(nullptr); break;
        case nlohmann::detail::value_t::string: strm->write(data.get_ref<string const&>()); break;
        case nlohmann::detail::value_t::boolean: strm->write(data.get<bool>()); break;
        case nlohmann::detail::value_t::number_integer: strm->write(data.get<int64_t>()); break;
        case nlohmann::detail::value_t::number_unsigned: strm->write(data.get<uint64_t>()); break;
        case nlohmann::detail::value_t::number_float: strm->write(data.get<double>()); break;

        case nlohmann::detail::value_t::binary: {
            auto& bin = data.get_ref<nlohmann::json::binary_t const&>();
            strm->binary_push(bin.size());
            strm->binary_write_some({bin.data(), bin.size()});
            strm->binary_pop();
        } break;

        case nlohmann::detail::value_t::object: {
            strm->object_push(data.size());
            for (auto& [key, value] : data.items()) {
                strm->write_key_next();
                strm->write(key);
                _archive_recursive(strm, value);
            }
            strm->object_pop();
        } break;

        case nlohmann::detail::value_t::array: {
            strm->array_push(data.size());
            for (auto& value : data) {
                _archive_recursive(strm, value);
            }
            strm->array_pop();
        } break;

        default:
        case nlohmann::detail::value_t::discarded:
            throw std::logic_error{"Tried to write invalid json type"};
    }
}

inline void _restore_recursive(archive::if_reader* strm, nlohmann::json* pdata, string* keybuf)
{
    switch (strm->type_next()) {
        case entity_type::object:
        case entity_type::dictionary: {
            if (not strm->config.merge_on_read || not pdata->is_object())
                *pdata = nlohmann::json::object();

            auto key = strm->begin_object();
            while (not strm->should_break(key)) {
                strm->read_key_next();
                strm->read(*keybuf);

                _restore_recursive(strm, &(*pdata)[*keybuf], keybuf);
            }
            strm->end_object(key);
        } break;

        case entity_type::tuple:
        case entity_type::array: {
            if (not strm->config.merge_on_read || not pdata->is_array())
                *pdata = nlohmann::json::array();

            auto key = strm->begin_array();
            while (not strm->should_break(key)) {
                _restore_recursive(strm, &pdata->emplace_back(), keybuf);
            }
            strm->end_array(key);
        } break;

        case entity_type::binary: {
            if (not pdata->is_binary())
                *pdata = nlohmann::json::binary({});

            auto nread = strm->begin_binary();
            auto& bin = pdata->get_ref<nlohmann::json::binary_t&>();
            bin.resize(nread);

            strm->binary_read_some({bin.data(), bin.size()});
            strm->end_binary();
        } break;

        case entity_type::null: *pdata = nullptr, strm->read<nullptr_t>(); break;
        case entity_type::boolean: *pdata = strm->read<bool>(); break;
        case entity_type::integer: *pdata = strm->read<int64_t>(); break;
        case entity_type::floating_point: *pdata = strm->read<double>(); break;

        case entity_type::string: {
            if (not pdata->is_string()) { *pdata = ""; }
            strm->read(pdata->get_ref<string&>());
        } break;

        case entity_type::invalid:
            throw archive::error::reader_invalid_context{strm};
    }
}
}  // namespace cpph::refl::_detail

namespace nlohmann {
CPPH_REFL_DEFINE_PRIM_begin(json)
{
    CPPH_REFL_primitive_type(object);
    CPPH_REFL_primitive_archive(strm, data)
    {
        cpph::refl::_detail::_archive_recursive(strm, data);
    }
    CPPH_REFL_primitive_restore(strm, data)
    {
        std::string keybuf;
        cpph::refl::_detail::_restore_recursive(strm, data, &keybuf);
    }
}
CPPH_REFL_DEFINE_PRIM_end();
}  // namespace nlohmann
#endif
