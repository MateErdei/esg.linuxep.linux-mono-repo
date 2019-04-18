include(GoogleTest)
add_executable(TestTelemetryImpl
        TestTelemetryValue.cpp
        TestTelemetrySerialiser.cpp
        TestTelemetryObject.cpp
        )
target_include_directories(TestTelemetryImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestTelemetryImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(TestTelemetryImpl gtest gtest_main)
target_link_libraries(TestTelemetryImpl telemetryimpl threads)
gtest_discover_tests(TestTelemetryImpl)
