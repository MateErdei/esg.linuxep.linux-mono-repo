include(GoogleTest)
add_executable(TestWdctl
        TestWdctlBootstrap.cpp
        )
target_include_directories(TestWdctl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" )
target_include_directories(TestWdctl BEFORE PUBLIC "${gtest_SOURCE_DIR}"  "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}")
target_link_libraries(TestWdctl gtest gmock gtest_main)
target_link_libraries(TestWdctl wdctlimpl)
gtest_discover_tests(TestWdctl)
add_dependencies(TestWdctl wdctlimpl gmock)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
