include(GoogleTest)
add_executable(TestOSUtilities
        TestMacInfo.cpp
        TestSXLMachineID.cpp
        TestDnsLookup.cpp
        TestIPUtils.cpp
        TestLocalIP.cpp
        MockILocalIP.h
        MockDnsLookup.h)
target_include_directories(TestOSUtilities SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" ${CMAKE_SOURCE_DIR})
#target_include_directories(TestOSUtilities BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" )
target_link_libraries(TestOSUtilities ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY} osutilitiesimpl  processimpl threads resolv utilityimpl testhelpers)
gtest_discover_tests(TestOSUtilities)
SET_TARGET_PROPERTIES(TestOSUtilities
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )