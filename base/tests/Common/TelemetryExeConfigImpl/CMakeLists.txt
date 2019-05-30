
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)

add_executable(TelemetryExeConfigImplTests
        ConfigTests.cpp
        ProxyTests.cpp
        MessageRelayTests.cpp
        )

target_include_directories(TelemetryExeConfigImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ../../../thirdparty/nlohmann-json)
target_include_directories(TelemetryExeConfigImplTests BEFORE PUBLIC "${gtest_SOURCE_DIR}")

target_link_libraries(TelemetryExeConfigImplTests telemetryexeconfigimpl)
target_link_libraries(TelemetryExeConfigImplTests gtest gmock gtest_main -L"${CMAKE_BINARY_DIR}/libs" gcov)

gtest_discover_tests(TelemetryExeConfigImplTests)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
