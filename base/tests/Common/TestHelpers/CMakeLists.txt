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
        )
target_include_directories(TestHelpersTest SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" )
target_include_directories(TestHelpersTest BEFORE PUBLIC "${gtest_SOURCE_DIR}" ${PROJECT_SOURCE_DIR}/modules)
target_link_libraries(TestHelpersTest gtest gtest_main)
target_link_libraries(TestHelpersTest filesystemimpl testhelpers)
gtest_discover_tests(TestHelpersTest)