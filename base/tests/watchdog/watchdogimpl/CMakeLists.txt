include(GoogleTest)
add_executable(TestWatchdog
        WatchdogTests.cpp
        TestPluginProxy.cpp
        TestWatchdog.cpp
        TestWatchdogAndWdctl.cpp TestWatchdogServiceLine.cpp MockIWatchdogRequest.h)
target_include_directories(TestWatchdog SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include")
target_include_directories(TestWatchdog BEFORE PUBLIC "${gtest_SOURCE_DIR}"  "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}")
target_link_libraries(TestWatchdog gtest gmock gtest_main)
target_link_libraries(TestWatchdog applicationconfigurationimpl watchdogimpl wdctlimpl  testhelpers telemetryhelperimpl)
gtest_discover_tests(TestWatchdog)
add_dependencies(TestWatchdog watchdogimpl gmock)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")