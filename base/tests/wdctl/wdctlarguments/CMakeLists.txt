include(GoogleTest)
add_executable(TestWdctlArguments
        TestWdctlArguments.cpp
        )
target_include_directories(TestWdctlArguments SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" )
target_include_directories(TestWdctlArguments BEFORE PUBLIC "${gtest_SOURCE_DIR}"  "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}")
target_link_libraries(TestWdctlArguments gtest gmock gtest_main)
target_link_libraries(TestWdctlArguments wdctlarguments)
gtest_discover_tests(TestWdctlArguments)
add_dependencies(TestWdctlArguments wdctlimpl gmock)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
