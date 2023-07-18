include(GoogleTest)

add_executable(CommonPluginCommunicationImplTests
        TestPluginProxy.cpp
        MockSocketRequester.h
        )

target_include_directories(CommonPluginCommunicationImplTests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_include_directories(CommonPluginCommunicationImplTests BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE} ${CMAKE_SOURCE_DIR})

target_link_libraries(CommonPluginCommunicationImplTests
        ${GTEST_MAIN_LIBRARY}
        plugincommunicationimplcommon
        testhelpers
        logging)

gtest_discover_tests(CommonPluginCommunicationImplTests)
SET_TARGET_PROPERTIES(CommonPluginCommunicationImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
