include(GoogleTest)

add_executable(TestTelemetryHelperImpl
        TestTelemetryValue.cpp
        TestTelemetrySerialiser.cpp
        TestTelemetryObject.cpp
        TestTelemetryHelper.cpp
        TestTelemetryJsonToMap.cpp)
target_include_directories(TestTelemetryHelperImpl SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE} ${NLOHMANN_JSON_INCLUDE_DIR})
target_link_libraries(TestTelemetryHelperImpl testhelpers ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY})
target_link_libraries(TestTelemetryHelperImpl telemetryhelperimpl threads)
gtest_discover_tests(TestTelemetryHelperImpl)
SET_TARGET_PROPERTIES(TestTelemetryHelperImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
