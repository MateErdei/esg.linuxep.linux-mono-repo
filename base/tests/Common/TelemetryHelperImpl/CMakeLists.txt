include(GoogleTest)

add_executable(TestTelemetryHelperImpl
        TestTelemetryValue.cpp
        TestTelemetrySerialiser.cpp
        TestTelemetryObject.cpp
        TestTelemetryHelper.cpp
        TestTelemetryJsonToMap.cpp)
target_include_directories(TestTelemetryHelperImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ../../../thirdparty/nlohmann-json)
target_include_directories(TestTelemetryHelperImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(TestTelemetryHelperImpl gtest gtest_main)
target_link_libraries(TestTelemetryHelperImpl telemetryhelperimpl testhelpers threads)
gtest_discover_tests(TestTelemetryHelperImpl)
