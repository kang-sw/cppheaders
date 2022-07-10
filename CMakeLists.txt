cmake_minimum_required(VERSION 3.16)

# -------------------------------------------------------------------------------------------------
project(cppheaders)
add_library(${PROJECT_NAME} INTERFACE)
add_library(kang-sw::cpph::core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE include)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

if (MSVC)
    target_compile_options(
            ${PROJECT_NAME}

            INTERFACE
            /utf-8
            /FIciso646)
endif ()

# -------------------------------------------------------------------------------------------------
project(cppheaders.json)
add_library(${PROJECT_NAME} STATIC)
add_library(kang-sw::cpph::json ALIAS ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PUBLIC cppheaders)

target_sources(
        ${PROJECT_NAME}

        PUBLIC
        src/json.cpp
        src/nlohmann_json.cpp)

# -------------------------------------------------------------------------------------------------
project(cppheaders.localization)
add_library(${PROJECT_NAME} STATIC)
add_library(kang-sw::cpph::localization ALIAS ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PUBLIC cppheaders.json)

target_sources(
        ${PROJECT_NAME}

        PRIVATE
        src/localize.cpp)

# -------------------------------------------------------------------------------------------------
add_subdirectory(tests)


