project(cppheaders_test_suit)

add_executable(
        ${PROJECT_NAME}

        doctest.cpp

        test-algorithm.cpp
        test-queue.cpp
        test-functional.cpp
        test-math.cpp
)

target_compile_definitions(
        ${PROJECT_NAME}

        PRIVATE
        -Dcpph=CPPHEADERS_NS_
)

target_compile_features(
        ${PROJECT_NAME}

        PRIVATE
        cxx_std_17
)

target_include_directories(
        ${PROJECT_NAME}

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if (TARGET nlohmann::json)
    message("[${PROJECT_NAME}]: nlhomann::json found. linking dependency ...")

    target_link_libraries(
            ${PROJECT_NAME}

            PRIVATE
            nlohmann::json
    )
endif ()

if (MSVC)
    add_compile_options(/utf-8)
endif ()
