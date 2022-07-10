cmake_minimum_required(VERSION 3.16)

# -------------------------------------------------------------------------------------------------
project(cppheaders)
add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE include)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

if (MSVC)
    target_compile_options(
            ${PROJECT_NAME}

            INTERFACE
            /utf-8
            /FIciso646)
endif ()

add_library(kang-sw::cpph::core ALIAS ${PROJECT_NAME})

# -------------------------------------------------------------------------------------------------
project(cppheaders.refl-extension)
add_library(${PROJECT_NAME} STATIC)
target_link_libraries(${PROJECT_NAME} PUBLIC cppheaders)
target_sources(${PROJECT_NAME} PRIVATE src/nlohmann_json.cpp)

if (TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
endif ()

add_library(kang-sw::cpph::refl-extension ALIAS ${PROJECT_NAME})

# -------------------------------------------------------------------------------------------------
project(cppheaders.localization)
add_library(${PROJECT_NAME} STATIC)
target_link_libraries(${PROJECT_NAME} PUBLIC cppheaders)
target_sources(${PROJECT_NAME} PRIVATE src/localize.cpp)

add_library(kang-sw::cpph::localization ALIAS ${PROJECT_NAME})

# -------------------------------------------------------------------------------------------------
add_subdirectory(tests)


