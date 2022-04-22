include(GoogleTest)
add_executable(PluginCommunicationImplTests
        TestPluginServerCallbackHandler.cpp
        TestPluginManager.cpp
        MockPluginServerCallback.h
        ../../Common/PluginApiImpl/MockedPluginApiCallback.h
        ../../Common/PluginCommunicationImpl/MockSocketRequester.h
        ../../Common/PluginApiImpl/TestCompare.h
        ../../Common/PluginApiImpl/TestCompare.cpp
        )
target_include_directories(PluginCommunicationImplTests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_include_directories(PluginCommunicationImplTests BEFORE PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(PluginCommunicationImplTests ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY})
target_link_libraries(PluginCommunicationImplTests plugincommunicationimpl testhelpers logging)
gtest_discover_tests(PluginCommunicationImplTests)
SET_TARGET_PROPERTIES(PluginCommunicationImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )