
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)
add_executable(TelemetryTests
        BaseTelemetryReporterTests.cpp
        TelemetryProcessorHttpSenderTests.cpp
        MockHttpSender.h
        SystemTelemetryCollectorImplTests.cpp
        ../Common/Helpers/MockProcess.h
        MockSystemTelemetryCollector.h
        SystemTelemetryReporterTests.cpp
        MockTelemetryProvider.h
        TelemetryProcessorTests.cpp
        TestTelemetryUtils.cpp
        PluginTelemetryReporterTests.cpp
        MockPluginProxy.h)

target_include_directories(TelemetryTests SYSTEM BEFORE PUBLIC ${PROTOBUF_INCLUDE_DIR} ${GTEST_INCLUDE} ${GMOCK_INCLUDE} ${CURL_INCLUDE_DIR})
target_include_directories(TelemetryTests BEFORE PUBLIC "${PROJECT_SOURCE_DIR}" ../../thirdparty/nlohmann-json)

target_link_libraries(TelemetryTests
        ${GTEST_LIBRARY}
        ${GTEST_MAIN_LIBRARY}
        ${}
        -L"${CMAKE_BINARY_DIR}/libs"
        ssl
        crypto
        telemetryimpl
        telemetryconfigimpl
        testhelpers
        obfuscationimpl
        processimpl
        xmlutilities
        threads
        gcov)

gtest_discover_tests(TelemetryTests)

SET_TARGET_PROPERTIES(TelemetryTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
