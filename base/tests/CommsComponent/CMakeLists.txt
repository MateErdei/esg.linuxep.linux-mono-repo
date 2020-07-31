include(GoogleTest)
add_executable(TestCommsComponent
        TestAsyncMessager.cpp
        TestSplitProcesses.cpp
        TestMonitorDir.cpp
        TestCommsMsg.cpp
        TestConfigurator.cpp
        TestCommsDistributor.cpp
        TestCommsConfig.cpp
        MockOtherSideApi.h
        CommsMsgUtils.h
        CommsMsgUtils.cpp
        TestHttpRequester.cpp
        ScopedFilePermissionsReplaceAndRestore.h
        ScopedFilePermissionsReplaceAndRestore.cpp
        )

target_include_directories(TestCommsComponent SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestCommsComponent BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" ${CMAKE_SOURCE_DIR})
target_link_libraries(TestCommsComponent gtest gmock gtest_main commscomponentimpl testhelpers utilityimpl)
gtest_discover_tests(TestCommsComponent)
