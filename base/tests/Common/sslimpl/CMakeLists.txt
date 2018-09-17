include(GoogleTest)
add_executable(TestSslImpl
        TestMd5Calc.cpp)
target_include_directories(TestSslImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestSslImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}")
target_link_libraries(TestSslImpl gtest gtest_main gmock sslimpl )
gtest_discover_tests(TestSslImpl)
