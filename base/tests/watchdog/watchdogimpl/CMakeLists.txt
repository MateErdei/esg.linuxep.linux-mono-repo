include(GoogleTest)
add_executable(TestWatchdog
        WatchdogTests.cpp
        TestPluginProxy.cpp
        TestWatchdog.cpp
        TestWatchdogAndWdctl.cpp TestWatchdogServiceLine.cpp MockIWatchdogRequest.h)
target_include_directories(TestWatchdog SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_include_directories(TestWatchdog BEFORE PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(TestWatchdog ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY})
target_link_libraries(TestWatchdog applicationconfigurationimpl watchdogimpl wdctlimpl  testhelpers telemetryhelperimpl)
gtest_discover_tests(TestWatchdog)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

SET_TARGET_PROPERTIES(TestWatchdog
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )