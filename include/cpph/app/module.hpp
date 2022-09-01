#pragma once
#include <cpph/std/string_view>

namespace cpph::os {
class module
{
    char chnk_[80];

   public:
    module(const module&) = delete;
    module& operator=(const module&) = delete;

   public:
    explicit module(string_view path) noexcept;
    ~module() noexcept;
    bool is_loaded();

    template <class Signature>
    Signature* load_symbol(string_view name) noexcept
    {
        return (Signature*)load_symbol_(name);
    }

   private:
    void* load_symbol_(string_view name) noexcept;
};
}  // namespace cpph::os
