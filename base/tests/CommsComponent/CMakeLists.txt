include(GoogleTest)
add_executable(TestCommsComponent
        TestAsyncMessager.cpp
        )
target_include_directories(TestCommsComponent SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestCommsComponent BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" ${CMAKE_SOURCE_DIR})
target_link_libraries(TestCommsComponent gtest gmock gtest_main commscomponentimpl)
gtest_discover_tests(TestCommsComponent)
