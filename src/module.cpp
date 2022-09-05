#include "cpph/app/module.hpp"

#include <cstring>

#include "cpph/helper/macros.hxx"
#include "cpph/utility/templates.hxx"

#ifdef _WIN32
#    include <cpph/std/filesystem>

#    include <Windows.h>

namespace cpph::os {
struct module_body {
    HINSTANCE hLib;
};

module::module(string_view pathstr) noexcept :chnk_{}
{
    static_assert(sizeof(module_body) < sizeof(module::chnk_));
    auto body = ((module_body*)chnk_);

    if (*(pathstr.data() + pathstr.size()) != '\0')  // non-zero-terminated
        pathstr = CPPH_ALLOCA_CSTR(pathstr);

    auto path = fs::path{pathstr};
    std::error_code ec;
    if (not path.is_absolute()) { path = fs::absolute(path, ec); }
    if (ec) { return; }  // If path is invalid ...

    body->hLib = ::LoadLibraryExW(path.c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
}

void* module::load_symbol_(string_view name) noexcept
{
    if (not is_loaded())
        return nullptr;

    if (*(name.data() + name.size()) != '\0')  // non-zero-terminated
        name = CPPH_ALLOCA_CSTR(name);

    auto body = ((module_body*)chnk_);
    auto proc = ::GetProcAddress(body->hLib, name.data());

    return (void*)proc;
}

bool module::is_loaded()
{
    return ((module_body*)chnk_)->hLib != nullptr;
}

module::~module() noexcept
{
    auto body = ((module_body*)chnk_);
    if (auto h_module = exchange(body->hLib, nullptr)) {
        ::FreeLibrary(h_module);
    }
}
}  // namespace cpph::os

#elif __unix__
#    include <dlfcn.h>

namespace cpph::os {
struct module_body {
    void* h_module;
};

module::module(string_view path) noexcept :chnk_{}
{
    static_assert(sizeof(module_body) < sizeof(module::chnk_));
    auto body = ((module_body*)chnk_);

    if (*(path.data() + path.size()) != '\0')  // non-zero-terminated
        path = CPPH_ALLOCA_CSTR(path);

    body->h_module = dlopen(path.data(), RTLD_LAZY | RTLD_LOCAL);
}

void* module::load_symbol_(string_view name) noexcept
{
    if (not is_loaded())
        return nullptr;

    if (*(name.data() + name.size()) != '\0')  // non-zero-terminated
        name = CPPH_ALLOCA_CSTR(name);

    auto body = ((module_body*)chnk_);
    return dlsym(body->h_module, name.data());
}

bool module::is_loaded()
{
    return ((module_body*)chnk_)->h_module != nullptr;
}

module::~module() noexcept
{
    auto body = ((module_body*)chnk_);
    if (auto h_module = exchange(body->h_module, nullptr)) {
        dlclose(h_module);
    }
}

}  // namespace cpph::os
#elif __APPLE__
// TODO

#endif
