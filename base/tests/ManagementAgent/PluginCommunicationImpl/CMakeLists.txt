include(GoogleTest)
add_executable(PluginCommunicationImplTests
        TestPluginServerCallbackHandler.cpp
        TestPluginManager.cpp
        TestPluginProxy.cpp
        MockPluginServerCallback.h
        MockSocketRequester.h
        ../../../tests/Common/PluginApiImpl/TestCompare.h
        ../../../tests/Common/PluginApiImpl/TestCompare.cpp
        )
target_include_directories(PluginCommunicationImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(PluginCommunicationImplTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ../../../)
target_link_libraries(PluginCommunicationImplTests gtest gtest_main gmock)
target_link_libraries(PluginCommunicationImplTests plugincommunicationimpl testhelpers logging)
gtest_discover_tests(PluginCommunicationImplTests)