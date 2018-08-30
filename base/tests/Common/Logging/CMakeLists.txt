
include(GoogleTest)
add_executable(TestLoggingSetup
        TestLoggingSetup.cpp)
target_include_directories(TestLoggingSetup SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestLoggingSetup BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(TestLoggingSetup gtest gtest_main)
target_link_libraries(TestLoggingSetup logging)

gtest_discover_tests(TestLoggingSetup)
