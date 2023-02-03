
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)

add_executable(ResponseActionsTests
        TestPluginAdapter.cpp
)

target_include_directories(ResponseActionsTests SYSTEM BEFORE PUBLIC ${PROTOBUF_INCLUDE_DIR} "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" )
target_include_directories(ResponseActionsTests BEFORE PUBLIC "${PROJECT_SOURCE_DIR}" ../../thirdparty/nlohmann-json)

target_link_libraries(ResponseActionsTests ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY} -L"${CMAKE_BINARY_DIR}/libs" testhelpers responseplugin)

gtest_discover_tests(ResponseActionsTests)

SET_TARGET_PROPERTIES(ResponseActionsTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")