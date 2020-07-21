include(GoogleTest)
add_executable(TestSecurityUtils
        TestSecurityUtils.cpp)
target_include_directories(TestSecurityUtils SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestSecurityUtils BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}"  ${CMAKE_SOURCE_DIR})
target_link_libraries(TestSecurityUtils gtest gtest_main gmock testhelpers securityutils)
target_link_libraries(TestSecurityUtils)
gtest_discover_tests(TestSecurityUtils)