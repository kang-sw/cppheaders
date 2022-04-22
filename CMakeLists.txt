cmake_minimum_required(VERSION 3.16)

# -------------------------------------------------------------------------------------------------
project(cppheaders)
add_library(${PROJECT_NAME} STATIC)

target_sources(
        ${PROJECT_NAME}

        PUBLIC
        src/json.cpp
)

target_compile_features(
        ${PROJECT_NAME}

        PUBLIC
        cxx_std_17
)

target_include_directories(
        ${PROJECT_NAME}

        PUBLIC
        include
)

if (MSVC)
    target_compile_options(
            ${PROJECT_NAME}

            PUBLIC
            /utf-8
            /FIciso646
    )
endif ()

# -------------------------------------------------------------------------------------------------
add_subdirectory(tests)
