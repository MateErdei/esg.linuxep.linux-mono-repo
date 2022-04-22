include(GoogleTest)
add_executable(PluginRegistryImplTests TestPluginRegistryImpl.cpp)
target_include_directories(PluginRegistryImplTests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${PROTOBUF_INCLUDE_DIR})
target_include_directories(PluginRegistryImplTests BEFORE PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(PluginRegistryImplTests
        ${GTEST_LIBRARY}
        ${GTEST_MAIN_LIBRARY}
        pluginregistryimpl
        ${Protobuf_LIBRARIES}
        applicationconfigurationimpl
        testhelpers)

gtest_discover_tests(PluginRegistryImplTests)
SET_TARGET_PROPERTIES(PluginRegistryImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )