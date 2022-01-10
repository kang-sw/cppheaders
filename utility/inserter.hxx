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
#include <stdexcept>
#include <type_traits>

#include "../__namespace__"

namespace CPPHEADERS_NS_ {
template <typename Ty_>
class polymorphic_inserter
{
   public:
    virtual ~polymorphic_inserter() noexcept = default;
    polymorphic_inserter& operator*() noexcept { return *this; };
    polymorphic_inserter& operator++(int) noexcept { return *this; }
    polymorphic_inserter& operator++() noexcept { return *this; }

   public:
    polymorphic_inserter& operator=(Ty_&& other)
    {
        this->assign(std::move(other));
        return *this;
    };
    polymorphic_inserter& operator=(Ty_ const& other)
    {
        this->assign(other);
        return *this;
    };

   private:
    virtual void assign(Ty_&& other)      = 0;
    virtual void assign(Ty_ const& other) = 0;
};

template <typename Range_>
auto back_inserter(Range_& range)
{
    using value_type
            = std::remove_const_t<
                    std::remove_reference_t<
                            decltype(*std::begin(std::declval<Range_>()))>>;

    class _iterator_impl : polymorphic_inserter<value_type>
    {
       public:
        Range_* _range;

       private:
        void assign(value_type&& other) override
        {
            _range->insert(_range->end(), std::move(other));
        }
        void assign(const value_type& other) override
        {
            _range->insert(_range->end(), other);
        }
    } iter{&range};

    return iter;
}

template <typename Range_>
auto array_inserter(Range_& range, size_t offset = 0)
{
    using value_type
            = std::remove_const_t<
                    std::remove_reference_t<
                            decltype(*std::begin(std::declval<Range_>()))>>;

    class _iterator_impl : polymorphic_inserter<value_type>
    {
       public:
        value_type* _where;
        value_type* _end;

       private:
        void assign(value_type&& other) override
        {
            if (_where >= _end)
                throw std::out_of_range{"array out of range"};
            *_where++ = std::move(other);
        }
        void assign(const value_type& other) override
        {
            if (_where >= _end)
                throw std::out_of_range{"array out of range"};
            *_where++ = other;
        }
    } iter{&*std::data(range) + offset, &*std::data(range) + std::size(range)};

    return iter;
}
}  // namespace CPPHEADERS_NS_