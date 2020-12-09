
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)

add_executable(TelemetrySchedulerTests
        TaskQueueTests.cpp
        SchedulerProcessorTests.cpp
        DerivedSchedulerProcessor.h
        SchedulerStatusTests.cpp
        MockTaskQueue.h
        SleepyThreadTests.cpp)

target_include_directories(TelemetrySchedulerTests SYSTEM BEFORE PUBLIC ${PROTOBUF_INCLUDE_DIR} "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TelemetrySchedulerTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}" ../../thirdparty/nlohmann-json)

target_link_libraries(TelemetrySchedulerTests gtest gmock gtest_main -L"${CMAKE_BINARY_DIR}/libs" ssl crypto testhelpers telemetryschedulerimpl)

gtest_discover_tests(TelemetrySchedulerTests)

SET_TARGET_PROPERTIES(TelemetrySchedulerTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
