include(GoogleTest)

add_executable(TelemetryConfigImplTests
        ConfigTests.cpp
        ProxyTests.cpp
        MessageRelayTests.cpp
        )

target_include_directories(TelemetryConfigImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ../../../thirdparty/nlohmann-json)
target_include_directories(TelemetryConfigImplTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}")

target_link_libraries(TelemetryConfigImplTests telemetryconfigimpl)
target_link_libraries(TelemetryConfigImplTests gtest gmock gtest_main gcov)

gtest_discover_tests(TelemetryConfigImplTests)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
