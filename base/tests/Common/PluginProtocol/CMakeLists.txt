include(GoogleTest)
add_executable(TestPluginProtocol
        DataMessageTests.cpp)
target_include_directories(TestPluginProtocol SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_include_directories(TestPluginProtocol
        BEFORE PUBLIC
        ${GTEST_INCLUDE}
        ${GMOCK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}
        )
target_link_libraries(TestPluginProtocol
        ${GTEST_LIBRARY}
        ${GTEST_MAIN_LIBRARY}
       )
target_link_libraries(TestPluginProtocol pluginprotocol testhelpers logging)
gtest_discover_tests(TestPluginProtocol)
SET_TARGET_PROPERTIES(TestPluginProtocol
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )