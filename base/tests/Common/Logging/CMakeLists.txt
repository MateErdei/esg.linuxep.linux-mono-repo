
include(GoogleTest)

# In the presence of both ASAN and coverage, we cannot use these tests because there is a conflict
if (GCOV_ACTIVE)
    SET( TestLoggingSources "")
else()
    SET( TestLoggingSources TestLoggerConfig.cpp)
endif()

add_executable(TestLoggingSetup
        TestLoggingSetup.cpp
        ${TestLoggingSources}
        )
target_include_directories(TestLoggingSetup SYSTEM BEFORE PUBLIC ${GMOCK_INCLUDE} ${GTEST_INCLUDE})
target_include_directories(TestLoggingSetup PUBLIC "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/modules" ${LOG4CPLUS_INCLUDE_DIR} )
target_link_libraries(TestLoggingSetup ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
target_link_libraries(TestLoggingSetup testhelpers logging)

gtest_discover_tests(TestLoggingSetup)
SET_TARGET_PROPERTIES(TestLoggingSetup
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

