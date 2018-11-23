include(GoogleTest)
add_executable(DirectoryWatcherImplTests
        TestDirectoryWatcherImpl.cpp TestRealiNotifyDirectoryWatcher.cpp
        MockiNotifyWrapper.h DummyDirectoryWatcherListener.h
        ../../ManagementAgent/McsRouterPluginCommunicationImpl/MockPluginManager.h
        ../Logging/TestConsoleLoggingSetup.cpp
        ../Logging/TestConsoleLoggingSetup.h
        )
target_include_directories(DirectoryWatcherImplTests SYSTEM BEFORE PUBLIC
        "${gtest_SOURCE_DIR}/include"
        "${gmock_SOURCE_DIR}/include"
        ${LOG4CPLUS_INCLUDE_DIR}
        )
target_include_directories(DirectoryWatcherImplTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}")
target_link_libraries(DirectoryWatcherImplTests gtest gtest_main gmock
            directorywatcherimpl threads zeromqwrapperimpl testhelpers
            logging
            ${LOG4CPLUS_LIBRARY}
        )
gtest_discover_tests(DirectoryWatcherImplTests)
