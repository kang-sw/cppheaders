#pragma once
#include <type_traits>

#include "../__namespace__.h"

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
        void assign(value_type const& other) override
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
        void assign(value_type const& other) override
        {
            if (_where >= _end)
                throw std::out_of_range{"array out of range"};
            *_where++ = other;
        }
    } iter{&*std::data(range) + offset, &*std::data(range) + std::size(range)};

    return iter;
}
}  // namespace CPPHEADERS_NS_