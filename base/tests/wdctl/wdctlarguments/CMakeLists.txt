include(GoogleTest)
add_executable(TestWdctlArguments
        TestWdctlArguments.cpp
        )
target_include_directories(TestWdctlArguments BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" "${CMAKE_SOURCE_DIR}")
target_link_libraries(TestWdctlArguments ${GTEST_LIBRARY} ${GMOCK_LIBRARY} ${GTEST_MAIN_LIBRARY} pthread)
target_link_libraries(TestWdctlArguments wdctlarguments)
gtest_discover_tests(TestWdctlArguments)
add_dependencies(TestWdctlArguments wdctlimpl)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
SET_TARGET_PROPERTIES(TestWdctlArguments
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )