include(GoogleTest)

add_executable(TestHelpersTest
        TestTempDir.cpp
        TempDirBaseClassTest.cpp
        TempDirBaseClassTest.h
        ExampleTempDirSuiteTest.cpp
        ExampleTempDirSuiteTest.h
        TestTempDirBaseClassTest.cpp
        TestExampleTempDirSuiteTest.cpp
        ExecutionSynchronizerDemoTests.cpp
        TestFakeTimeUtils.cpp
        MockApiBaseServicesTests.cpp
        CurlWrapperTests.cpp
        ${PROJECT_SOURCE_DIR}/modules/Common/CurlWrapper/CurlWrapper.h
        ${PROJECT_SOURCE_DIR}/modules/Common/CurlWrapper/CurlWrapper.cpp
        )
target_include_directories(TestHelpersTest SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE} ${CMAKE_SOURCE_DIR} ${CURL_INCLUDE_DIR})
target_link_libraries(TestHelpersTest ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
target_link_libraries(TestHelpersTest filesystemimpl testhelpers utilityimpl ${CURL_LIBRARY})
gtest_discover_tests(TestHelpersTest)
SET_TARGET_PROPERTIES(TestHelpersTest
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )