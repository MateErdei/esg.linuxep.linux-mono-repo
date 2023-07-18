include(GoogleTest)
add_executable(TestSslImpl
        TestDigest.cpp)
target_include_directories(TestSslImpl SYSTEM BEFORE PUBLIC)
target_include_directories(TestSslImpl BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}")
target_link_libraries(TestSslImpl ${GTEST_MAIN_LIBRARY} pthread sslimpl)
gtest_discover_tests(TestSslImpl)
SET_TARGET_PROPERTIES(TestSslImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )