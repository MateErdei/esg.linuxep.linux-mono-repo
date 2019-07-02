include(GoogleTest)

add_executable(CommonPluginCommunicationImplTests
        TestPluginProxy.cpp
        MockSocketRequester.h
        )

target_include_directories(CommonPluginCommunicationImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(CommonPluginCommunicationImplTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_link_libraries(CommonPluginCommunicationImplTests gtest gtest_main gmock)
target_link_libraries(CommonPluginCommunicationImplTests plugincommunicationimplcommon testhelpers logging)
gtest_discover_tests(CommonPluginCommunicationImplTests)
