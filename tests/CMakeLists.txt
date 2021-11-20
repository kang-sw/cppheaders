project(cppheaders_test_suit)

add_executable(
        ${PROJECT_NAME}

        doctest.cpp

        test-base64.cpp
)

target_compile_definitions(
        ${PROJECT_NAME}

        PRIVATE
        -DKANGSW_TEMPLATE_NAMESPACE=cpph
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
