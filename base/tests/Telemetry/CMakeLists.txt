
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)
add_executable(TelemetryTests
        TelemetryTests.cpp
        MockHttpSender.h
        SystemTelemetryCollectorImplTests.cpp
        ../Common/ProcessImpl/MockProcess.h
        MockSystemTelemetryCollector.h
        SystemTelemetryReporterTests.cpp)

target_include_directories(TelemetryTests PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_include_directories(TelemetryTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ${CURL_INCLUDE_DIR})
target_include_directories(TelemetryTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}")

target_link_libraries(TelemetryTests gtest gmock gtest_main -L"${CMAKE_BINARY_DIR}/libs" ssl crypto telemetryimpl testhelpers)
target_link_libraries(TelemetryTests processimpl threads)
target_link_libraries(TelemetryTests gcov)
gtest_discover_tests(TelemetryTests)

SET_TARGET_PROPERTIES(TelemetryTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
