include(GoogleTest)
add_executable(TestDatatypes
        TestStringUtils.cpp)
target_include_directories(TestDatatypes SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestDatatypes BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}")
target_link_libraries(TestDatatypes gtest gtest_main gmock)
target_link_libraries(TestDatatypes)
gtest_discover_tests(TestDatatypes)
