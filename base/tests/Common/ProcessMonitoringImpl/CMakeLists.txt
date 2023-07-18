include(GoogleTest)
add_executable(ProcessMonitoringImplTests TestProcessMonitoring.cpp TestProcessProxy.cpp)
target_include_directories(ProcessMonitoringImplTests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE} ${CMAKE_SOURCE_DIR})
target_link_libraries(ProcessMonitoringImplTests ${GTEST_MAIN_LIBRARY})
target_link_libraries(ProcessMonitoringImplTests processmonitoringimpl threads testhelpers)
gtest_discover_tests(ProcessMonitoringImplTests)
SET_TARGET_PROPERTIES(ProcessMonitoringImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )