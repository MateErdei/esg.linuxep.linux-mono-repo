include(GoogleTest)
add_executable(TestPolicy
        TestProxyCredentials.cpp
        )
target_include_directories(TestPolicy SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${PROTOBUF_INCLUDE_DIR})
target_include_directories(TestPolicy BEFORE PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(TestPolicy
        ${GTEST_LIBRARY}
        ${GTEST_MAIN_LIBRARY}
        policy
        testhelpers)

gtest_discover_tests(TestPolicy)
SET_TARGET_PROPERTIES(TestPolicy
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
