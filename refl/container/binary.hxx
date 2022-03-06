
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
#include "../__namespace__"
#include "../detail/binary_fwd.hxx"
#include "../detail/primitives.hxx"

/*
 * Binary
 */
namespace CPPHEADERS_NS_ {

template <typename Container_>
refl::object_metadata_ptr
initialize_object_metadata(refl::type_tag<binary<Container_>>)
{
    using binary_type = binary<Container_>;

    static struct manip_t : refl::templated_primitive_control<binary_type>
    {
        refl::entity_type type() const noexcept override
        {
            return refl::entity_type::binary;
        }
        void impl_archive(archive::if_writer* strm,
                          const binary_type& pvdata,
                          refl::object_metadata_t desc,
                          refl::optional_property_metadata prop) const override
        {
            auto data = &pvdata;

            if constexpr (not binary_type::is_container)
            {
                *strm << const_buffer_view{data, 1};
            }
            else if constexpr (binary_type::is_contiguous)  // list, set, etc ...
            {
                *strm << const_buffer_view{*data};
            }
            else
            {
                using value_type = typename Container_::value_type;
                auto total_size  = sizeof(value_type) * std::size(*data);

                strm->binary_push(total_size);
                for (auto& elem : *data)
                {
                    strm->binary_write_some(const_buffer_view{&elem, 1});
                }
                strm->binary_pop();
            }
        }

        void impl_restore(archive::if_reader* strm,
                          binary_type* data,
                          refl::object_metadata_t desc,
                          refl::optional_property_metadata prop) const override
        {
            auto chunk_size             = strm->begin_binary();
            [[maybe_unused]] auto clean = cleanup([&] { strm->end_binary(); });

            if constexpr (not binary_type::is_container)
            {
                strm->binary_read_some(mutable_buffer_view{data, 1});
            }
            else
            {
                using value_type = typename Container_::value_type;

                auto elem_count_verified =
                        [&] {
                            if (chunk_size % sizeof(value_type) != 0)
                                throw refl::error::primitive{strm, "Binary alignment mismatch"};

                            return chunk_size / sizeof(value_type);
                        };

                if constexpr (binary_type::is_contiguous)
                {
                    if (chunk_size != ~size_t{})
                    {
                        if constexpr (refl::has_resize<Container_>)
                        {
                            // If it's dynamic, read all.
                            data->resize(elem_count_verified());
                        }

                        strm->binary_read_some({std::data(*data), std::size(*data)});
                    }
                    else  // if chunk size is not specified ...
                    {
                        if constexpr (refl::has_emplace_back<Container_>)
                            data->clear();

                        value_type elem_buf;
                        value_type* elem;

                        for (size_t idx = 0;; ++idx)
                        {
                            if constexpr (refl::has_emplace_back<Container_>)
                                elem = &elem_buf;
                            else if (idx < std::size(*data))
                                elem = std::data(*data) + idx;
                            else
                                break;

                            auto n = strm->binary_read_some(mutable_buffer_view{elem, 1});

                            if (n == sizeof *elem)
                            {
                                if constexpr (refl::has_emplace_back<Container_>)
                                    data->emplace_back(std::move(*elem));
                            }
                            else if (n == 0)
                            {
                                if constexpr (refl::has_emplace_back<Container_>)
                                    break;
                                else
                                    throw refl::error::primitive{strm, "missing data"};
                            }
                            else if (n != sizeof(value_type))
                            {
                                throw refl::error::primitive{strm, "binary data alignment mismatch"};
                            }
                        }
                    }
                }
                else
                {
                    if (chunk_size != ~size_t{})
                    {
                        auto elemsize = elem_count_verified();

                        if constexpr (refl::has_reserve_v<Container_>)
                        {
                            data->reserve(elemsize);
                        }

                        data->clear();
                        value_type* mutable_data = {};
                        for (auto idx : perfkit::count(elemsize))
                        {
                            if constexpr (refl::has_emplace_back<Container_>)  // maybe vector, list, deque ...
                                mutable_data = &data->emplace_back();
                            else if constexpr (refl::has_emplace_front<Container_>)  // maybe forward_list
                                mutable_data = &data->emplace_front();
                            else if constexpr (refl::has_emplace<Container_>)  // maybe set
                                mutable_data = &*data->emplace().first;
                            else
                                Container_::ERROR_INVALID_CONTAINER;

                            strm->binary_read_some({mutable_data, 1});
                        }
                    }
                    else  // chunk size not specified
                    {
                        data->clear();
                        value_type chunk;

                        for (;;)
                        {
                            auto n = strm->binary_read_some({&chunk, 1});

                            if (n == 0)
                                break;
                            else if (n != sizeof chunk)
                                throw refl::error::primitive{strm, "binary data alignment mismatch"};

                            if constexpr (refl::has_emplace_back<Container_>)  // maybe vector, list, deque ...
                                data->emplace_back(std::move(chunk));
                            else if constexpr (refl::has_emplace_front<Container_>)  // maybe forward_list
                                data->emplace_front(std::move(chunk));
                            else if constexpr (refl::has_emplace<Container_>)  // maybe set
                                data->emplace(std::move(chunk));
                            else
                                Container_::ERROR_INVALID_CONTAINER;
                        }
                    }
                }
            }
        }
    } manip;

    return refl::object_metadata::primitive_factory::define(sizeof(Container_), &manip);
}

/*
 * TODO: Trivial binary with contiguous memory chunk, which can apply read/write adapters
 */

}  // namespace CPPHEADERS_NS_