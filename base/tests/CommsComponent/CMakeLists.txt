include(GoogleTest)
add_executable(TestCommsComponent
        TestAsyncMessager.cpp
        TestSplitProcesses.cpp
        TestMonitorDir.cpp
        TestCommsMsg.cpp
        TestConfigurator.cpp
        TestCommsDistributor.cpp
        TestCommsConfig.cpp
        TestNetworkSide.cpp
        MockOtherSideApi.h
        CommsMsgUtils.h
        CommsMsgUtils.cpp
        TestHttpRequester.cpp
        ScopedFilePermissionsReplaceAndRestore.h
        ScopedFilePermissionsReplaceAndRestore.cpp
        )

#target_include_directories(TestCommsComponent SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" )
target_include_directories(TestCommsComponent BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}"  ${CMAKE_SOURCE_DIR})
target_link_libraries(TestCommsComponent ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY} commscomponentimpl testhelpers utilityimpl)
gtest_discover_tests(TestCommsComponent)
SET_TARGET_PROPERTIES(TestCommsComponent
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
