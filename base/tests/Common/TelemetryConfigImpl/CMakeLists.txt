include(GoogleTest)

add_executable(TelemetryConfigImplTests
        ConfigTests.cpp
        ProxyTests.cpp
        MessageRelayTests.cpp
        )

target_include_directories(TelemetryConfigImplTests SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" ../../../thirdparty/nlohmann-json)

target_link_libraries(TelemetryConfigImplTests telemetryconfigimpl)
target_link_libraries(TelemetryConfigImplTests ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY})

gtest_discover_tests(TelemetryConfigImplTests)

SET_TARGET_PROPERTIES(TelemetryConfigImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
