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
        )
target_include_directories(TestHelpersTest SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_include_directories(TestHelpersTest BEFORE PUBLIC "${gtest_SOURCE_DIR}" ${PROJECT_SOURCE_DIR}/modules)
target_link_libraries(TestHelpersTest gtest gtest_main)
target_link_libraries(TestHelpersTest filesystemimpl testhelpers utilityimpl)
gtest_discover_tests(TestHelpersTest)