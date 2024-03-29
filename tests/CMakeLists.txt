project(cppheaders_test_suit)

add_executable(
        ${PROJECT_NAME}

        compile-test.cpp
        test-algorithm.cpp
        test-queue.cpp
        test-functional.cpp
        test-math.cpp
        test-thread.cpp
        test-streambuf.cpp
        test-templates.cpp
        test-rpc.cpp
        test-archive.cpp
        test-archive-2.cpp
        test-container.cpp
        test-event_queue.cpp
        test-pipeline.cpp
        test-event.cpp
)

if (NOT TARGET doctest_with_main)
    message("[${PROJECT_NAME}] doctest not found. Populating ...")

    include(FetchContent)
    FetchContent_Declare(doctest URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.8.zip)
    FetchContent_Populate(doctest)
    add_subdirectory(${doctest_SOURCE_DIR} ${doctest_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()

target_link_libraries(
        ${PROJECT_NAME}

        PRIVATE
        kang-sw::cpph::core
        kang-sw::cpph::refl-extension
        kang-sw::cpph::localization
        doctest_with_main
)

target_include_directories(
        ${PROJECT_NAME}

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/cpph
)

if (TARGET nlohmann_json::nlohmann_json)
    message("[${PROJECT_NAME}] nlohmann_json::nlohmann_json found. linking dependency ...")

    target_link_libraries(
            ${PROJECT_NAME}

            PRIVATE
            nlohmann_json::nlohmann_json
    )
endif ()

if (TARGET perfkit::asio)
    message("[${PROJECT_NAME}] perfkit::asio found. linking dependency ...")

    target_link_libraries(
            ${PROJECT_NAME}

            PRIVATE
            perfkit::asio
    )

    target_sources(
            ${PROJECT_NAME}

            PRIVATE
            test-rpc.cpp
    )
endif ()

if (MSVC)
    add_compile_options(/utf-8)
else ()
    target_link_libraries(
            ${PROJECT_NAME}

            PUBLIC
            pthread
    )
endif ()

if (FALSE)
    project(${PROJECT_NAME}-gcc-compilation)

    add_executable(
            ${PROJECT_NAME}

            test-gcccompile.cpp
    )

    target_compile_features(
            ${PROJECT_NAME}

            PRIVATE
            cxx_std_17
    )
endif ()
