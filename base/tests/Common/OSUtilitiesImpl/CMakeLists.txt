include(GoogleTest)
add_executable(TestOSUtilities
        TestMacInfo.cpp
        TestDnsLookup.cpp
        TestIPUtils.cpp
        TestLocalIP.cpp
        TestPlatformUtils.cpp
        MockILocalIP.h
        MockDnsLookup.h)
target_include_directories(TestOSUtilities SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" ${CMAKE_SOURCE_DIR})
target_link_libraries(TestOSUtilities ${GTEST_MAIN_LIBRARY} osutilitiesimpl processimpl threads resolv utilityimpl testhelpers xmlutilities)
gtest_discover_tests(TestOSUtilities)
SET_TARGET_PROPERTIES(TestOSUtilities
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )