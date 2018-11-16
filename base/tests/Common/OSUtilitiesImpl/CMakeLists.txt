include(GoogleTest)
add_executable(TestOSUtilities
        TestMacInfo.cpp TestSXLMachineID.cpp TestPidLockFile.cpp MockPidLockFileUtils.h )
target_include_directories(TestOSUtilities SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_include_directories(TestOSUtilities BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" )
target_link_libraries(TestOSUtilities gtest gtest_main gmock osutilitiesimpl  processimpl threads)
gtest_discover_tests(TestOSUtilities)
