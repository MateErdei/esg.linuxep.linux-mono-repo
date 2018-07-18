include(GoogleTest)
add_executable(McsRouterPluginCommunicationImplTests
        TaskQueueTests.cpp
        MockPluginManager.h
        )
target_include_directories(McsRouterPluginCommunicationImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(McsRouterPluginCommunicationImplTests BEFORE PUBLIC "${gtest_SOURCE_DIR}"  "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ../../../ ../../Common/TestHelpers)
target_link_libraries(McsRouterPluginCommunicationImplTests PUBLIC gtest gmock gtest_main mcsrouterplugincommunicationimpl directorywatcherimpl plugincommunicationimpl threads testhelpers)
gtest_discover_tests(McsRouterPluginCommunicationImplTests)
