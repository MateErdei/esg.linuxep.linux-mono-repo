
include(GoogleTest)
add_executable(TestLoggingSetup
        TestLoggingSetup.cpp
        TestLoggerConfig.cpp)
target_include_directories(TestLoggingSetup SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestLoggingSetup PUBLIC "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/modules" ${LOG4CPLUS_INCLUDE_DIR} )
target_link_libraries(TestLoggingSetup gtest gtest_main)
target_link_libraries(TestLoggingSetup testhelpers logging)

gtest_discover_tests(TestLoggingSetup)

add_executable(LoggerLimit
        LoggerLimit.cpp
        )
target_include_directories(LoggerLimit PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
target_link_libraries(LoggerLimit logging)