include(GoogleTest)
add_executable(McsRouterPluginCommunicationImplTests
        McsRouterPluginCommunicationImplTests.cpp
        MockPluginManager.h
        MockTaskQueue.h PolicyTaskTests.cpp TaskDirectoryListenerTests.cpp PolicyAndActionListenerTests.cpp ActionTaskTests.cpp)
target_include_directories(McsRouterPluginCommunicationImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(McsRouterPluginCommunicationImplTests BEFORE PUBLIC "${gtest_SOURCE_DIR}"  "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ../../../ ../../Common/TestHelpers)
target_link_libraries(McsRouterPluginCommunicationImplTests PUBLIC
            gtest gmock gtest_main
            mcsrouterplugincommunicationimpl directorywatcherimpl plugincommunicationimpl
            taskqueueimpl
        threads testhelpers)
gtest_discover_tests(McsRouterPluginCommunicationImplTests)
