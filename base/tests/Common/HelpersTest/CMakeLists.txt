include(GoogleTest)

add_executable(TestHelpersTest
        TestTempDir.cpp
        ../Helpers/TempDirBaseClassTest.cpp
        ../Helpers/TempDirBaseClassTest.h
        TestTempDirBaseClassTest.cpp
        ExecutionSynchronizerDemoTests.cpp
        TestFakeTimeUtils.cpp
        MockApiBaseServicesTests.cpp
        CurlWrapperTests.cpp
        )
target_include_directories(TestHelpersTest SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE} ${CMAKE_SOURCE_DIR})
target_link_libraries(TestHelpersTest ${GTEST_MAIN_LIBRARY} curlwrapper)
target_link_libraries(TestHelpersTest filesystemimpl testhelpers utilityimpl)
gtest_discover_tests(TestHelpersTest)
SET_TARGET_PROPERTIES(TestHelpersTest
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )