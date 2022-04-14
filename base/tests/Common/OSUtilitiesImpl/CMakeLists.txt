include(GoogleTest)
add_executable(TestOSUtilities
        TestMacInfo.cpp
        TestSXLMachineID.cpp
        TestDnsLookup.cpp
        TestIPUtils.cpp
        TestLocalIP.cpp
        TestPlatformUtils.cpp
        MockILocalIP.h
        MockDnsLookup.h)
target_include_directories(TestOSUtilities SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR} ../../../thirdparty/nlohmann-json)
target_link_libraries(TestOSUtilities gtest gtest_main gmock osutilitiesimpl  processimpl threads resolv utilityimpl testhelpers)
gtest_discover_tests(TestOSUtilities)
SET_TARGET_PROPERTIES(TestOSUtilities
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )