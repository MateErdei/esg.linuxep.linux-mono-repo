include(GoogleTest)
add_executable(PluginApiImplTests
        TestProtocol.cpp
        SingleResponseServer.h
        MockedPluginApiCallback.h
        PluginApiTests.cpp
        SingleManagementRequest.h
        PluginApiCallbackTests.cpp
        RawDataCallbackTests.cpp
        MessageBuilderTests.cpp
        TestCompare.h
        TestCompare.cpp
        PubSubReliabilityTests.cpp
        )
target_include_directories(PluginApiImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" )
message( "check"  "${gtest_SOURCE_DIR}"  "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include")
target_include_directories(PluginApiImplTests
        BEFORE PUBLIC "${gtest_SOURCE_DIR}"  "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include"
        PluginApiImpl ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/modules/Common/PluginApi ${CMAKE_SOURCE_DIR})
target_link_libraries(PluginApiImplTests gtest gmock gtest_main)
target_link_libraries(PluginApiImplTests pluginapiimpl reactorimpl threads pluginprotocol applicationconfigurationimpl testhelpers logging )
gtest_discover_tests(PluginApiImplTests)
add_dependencies(PluginApiImplTests gmock)