include(GoogleTest)
add_executable(TestPluginProtocol
        DataMessageTests.cpp)
target_include_directories(TestPluginProtocol SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" )
target_include_directories(TestPluginProtocol
        BEFORE PUBLIC "${gtest_SOURCE_DIR}"  "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include"
        ../Helpers ${CMAKE_SOURCE_DIR}/modules)
target_link_libraries(TestPluginProtocol gtest gmock gtest_main)
target_link_libraries(TestPluginProtocol pluginprotocol testhelpers logging)
gtest_discover_tests(TestPluginProtocol)
add_dependencies(TestPluginProtocol gmock)
