include(GoogleTest)
add_executable(TestPolicy
        TestProxyCredentials.cpp
        TestProxy.cpp
        TestALCPolicy.cpp
        TestSerialiseUpdateSettings.cpp
        TestUpdateSettings.cpp
        TestUpdateSettingsBase.h
        TestMCSPolicy.cpp)
target_include_directories(TestPolicy SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${LOG4CPLUS_INCLUDE_DIR} ${PROTOBUF_INCLUDE_DIR})
target_include_directories(TestPolicy BEFORE PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(TestPolicy
        ${GTEST_MAIN_LIBRARY}
        policy
        testhelpersobject)

gtest_discover_tests(TestPolicy)
SET_TARGET_PROPERTIES(TestPolicy
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
