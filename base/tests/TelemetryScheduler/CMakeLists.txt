
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)

add_executable(TelemetrySchedulerTests
        TelemetrySchedulerTests.cpp
        TaskQueueTests.cpp)

target_include_directories(TelemetrySchedulerTests PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_include_directories(TelemetrySchedulerTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TelemetrySchedulerTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}" ../../thirdparty/nlohmann-json)

target_link_libraries(TelemetrySchedulerTests gtest gmock gtest_main -L"${CMAKE_BINARY_DIR}/libs" ssl crypto telemetryschedulerimpl gcov)

gtest_discover_tests(TelemetrySchedulerTests)

SET_TARGET_PROPERTIES(TelemetrySchedulerTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
