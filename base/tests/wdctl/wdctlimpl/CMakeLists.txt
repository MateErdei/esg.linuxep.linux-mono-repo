include(GoogleTest)
add_executable(TestWdctl
        TestWdctlBootstrap.cpp
        )
target_include_directories(TestWdctl SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}")
target_include_directories(TestWdctl BEFORE PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(TestWdctl ${GTEST_MAIN_LIBRARY})
target_link_libraries(TestWdctl wdctlimpl)
gtest_discover_tests(TestWdctl)
add_dependencies(TestWdctl wdctlimpl)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
SET_TARGET_PROPERTIES(TestWdctl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )