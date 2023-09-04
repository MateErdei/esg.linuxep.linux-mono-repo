include(GoogleTest)
add_executable(DirectoryWatcherImplTests
        TestDirectoryWatcherImpl.cpp TestRealiNotifyDirectoryWatcher.cpp
        DummyDirectoryWatcherListener.h
        ../../ManagementAgent/MockPluginManager/MockPluginManager.h
        )
target_include_directories(DirectoryWatcherImplTests SYSTEM BEFORE PUBLIC
        ${GMOCK_INCLUDE}
        ${GTEST_INCLUDE}
        ${LOG4CPLUS_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}
        )
target_link_libraries(DirectoryWatcherImplTests ${GTEST_MAIN_LIBRARY}
        directorywatcherimpl threads zeromqwrapperimpl testhelpers
        logging
        ${LOG4CPLUS_LIBRARY}
        )
gtest_discover_tests(DirectoryWatcherImplTests)
SET_TARGET_PROPERTIES(DirectoryWatcherImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )