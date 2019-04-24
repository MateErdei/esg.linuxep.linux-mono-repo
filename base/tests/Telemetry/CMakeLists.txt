
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)
add_executable(TelemetryTests
        TelemetryTests.cpp
        )

target_include_directories(TelemetryTests PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_include_directories(TelemetryTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TelemetryTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}")

target_link_libraries(TelemetryTests gtest gmock gtest_main)
target_link_libraries(TelemetryTests telemetryimpl)
gtest_discover_tests(TelemetryTests)

SET_TARGET_PROPERTIES(TelemetryTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
