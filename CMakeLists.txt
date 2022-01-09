cmake_minimum_required(VERSION 3.16)

if (NOT PROJECT_NAME)
    message("Configuring CppHeaders as standalone project ...")

    project(cppheaders)
endif ()

if (MSVC)
    add_compile_options(/showIncludes)
endif ()

add_subdirectory(tests)
