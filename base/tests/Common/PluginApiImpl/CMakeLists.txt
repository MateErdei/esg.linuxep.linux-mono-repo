include(GoogleTest)
add_executable(PluginApiImplTests
        TestProtocol.cpp
        SingleResponseServer.h
        MockedPluginApiCallback.h
        PluginApiTests.cpp
        PluginCallBackHandlerTests.cpp
        SingleManagementRequest.h
        PluginApiCallbackTests.cpp
        MessageBuilderTests.cpp
        TestCompare.h
        TestCompare.cpp
        )
target_include_directories(PluginApiImplTests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE})
target_include_directories(PluginApiImplTests
        BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE}
        PluginApiImpl ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/modules/Common/PluginApi ${CMAKE_SOURCE_DIR})
target_link_libraries(PluginApiImplTests ${GMOCK_LIBRARY} ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} )
target_link_libraries(PluginApiImplTests pluginapiimpl reactorimpl threads pluginprotocol applicationconfigurationimpl testhelpers logging)
gtest_discover_tests(PluginApiImplTests)
SET_TARGET_PROPERTIES(PluginApiImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )