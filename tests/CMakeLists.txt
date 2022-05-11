project(cppheaders_test_suit)

add_executable(
        ${PROJECT_NAME}

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
)

if (NOT TARGET doctest_with_main)
    message("[${PROJECT_NAME}] doctest not found. Populating ...")

    include(FetchContent)
    FetchContent_Declare(doctest URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.8.zip)
    FetchContent_MakeAvailable(doctest)
endif ()

target_link_libraries(
        ${PROJECT_NAME}

        PRIVATE
        cppheaders
        doctest_with_main
)

target_include_directories(
        ${PROJECT_NAME}

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/cpph
)

if (TARGET nlohmann::json)
    message("[${PROJECT_NAME}] nlhomann::json found. linking dependency ...")

    target_link_libraries(
            ${PROJECT_NAME}

            PRIVATE
            nlohmann::json
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
