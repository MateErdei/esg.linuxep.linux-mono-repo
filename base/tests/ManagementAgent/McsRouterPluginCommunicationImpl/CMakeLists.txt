include(GoogleTest)
add_executable(McsRouterPluginCommunicationImplTests
        McsRouterPluginCommunicationImplTests.cpp
        ../MockPluginManager/MockPluginManager.h
        MockTaskQueue.h
        PolicyTaskTests.cpp
        TaskDirectoryListenerTests.cpp
        ActionTaskTests.cpp
        )

target_include_directories(McsRouterPluginCommunicationImplTests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_include_directories(McsRouterPluginCommunicationImplTests BEFORE PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(McsRouterPluginCommunicationImplTests
        ${GTEST_MAIN_LIBRARY}
        xmlutilities
        mcsrouterplugincommunicationimpl
        testhelpers
        )

gtest_discover_tests(McsRouterPluginCommunicationImplTests)
SET_TARGET_PROPERTIES(McsRouterPluginCommunicationImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )