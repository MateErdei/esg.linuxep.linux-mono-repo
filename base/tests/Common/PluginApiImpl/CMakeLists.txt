include(GoogleTest)
add_executable(PluginApiImplTests
        TestProtocol.cpp
        SingleResponseServer.h
        MockedPluginApiCallback.h
        PluginApiTests.cpp
        SingleManagementRequest.h
        PluginApiCallbackTests.cpp
        MockSensorDataCallback.h
        SensorDataCallbackTests.cpp
        MessageBuilderTests.cpp
        TestCompare.h
        TestCompare.cpp
        MockApiBaseServices.h
        )
target_include_directories(PluginApiImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" )
message( "check"  "${gtest_SOURCE_DIR}"  "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include")
target_include_directories(PluginApiImplTests
        BEFORE PUBLIC "${gtest_SOURCE_DIR}"  "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include"
        PluginApiImpl ../TestHelpers ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/modules/Common/PluginApi ../../../)
target_link_libraries(PluginApiImplTests gtest gmock gtest_main)
target_link_libraries(PluginApiImplTests pluginapiimpl reactorimpl threads pluginprotocol applicationconfigurationimpl testhelpers logging filepermissionsimpl)
gtest_discover_tests(PluginApiImplTests)
add_dependencies(PluginApiImplTests gmock)