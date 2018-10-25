include(GoogleTest)
add_library(testhelpers SHARED
        TempDir.h
        TempDir.cpp
        TempDirBaseClassTest.h
        TempDirBaseClassTest.cpp
        TestExecutionSynchronizer.h
        TestExecutionSynchronizer.cpp)
target_link_libraries(testhelpers PUBLIC filesystemimpl filepermissionsimpl)
target_include_directories(testhelpers SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" )


add_executable(TestHelpersTest TestTempDir.cpp ExampleTempDirSuiteTest.cpp ExampleTempDirSuiteTest.h TestTempDirBaseClassTest.cpp TestExampleTempDirSuiteTest.cpp TestExecutionSynchronizer.cpp TestExecutionSynchronizer.h )
target_include_directories(TestHelpersTest SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" )
target_include_directories(TestHelpersTest BEFORE PUBLIC "${gtest_SOURCE_DIR}" ${PROJECT_SOURCE_DIR}/modules)
target_link_libraries(TestHelpersTest gtest gtest_main)
target_link_libraries(TestHelpersTest filesystemimpl filepermissionsimpl testhelpers)
gtest_discover_tests(TestHelpersTest)