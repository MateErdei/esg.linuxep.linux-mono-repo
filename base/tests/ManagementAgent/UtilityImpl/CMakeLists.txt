include(GoogleTest)
add_executable(TestUtilitiesImpl
        TestUtilities.cpp
        )
target_include_directories(TestUtilitiesImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestUtilitiesImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_link_libraries(TestUtilitiesImpl gtest gtest_main gmock)
target_link_libraries(TestUtilitiesImpl managementagentutilityimpl testhelpers )

gtest_discover_tests(TestUtilitiesImpl)
