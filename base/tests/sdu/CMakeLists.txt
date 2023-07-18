
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)

add_executable(sduTests
        TaskQueueTests.cpp
        TestPluginUtils.cpp
        MockTaskQueue.h
)

target_include_directories(sduTests SYSTEM BEFORE PUBLIC ${PROTOBUF_INCLUDE_DIR} "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" )
target_include_directories(sduTests BEFORE PUBLIC "${PROJECT_SOURCE_DIR}" ../../thirdparty/nlohmann-json)

target_link_libraries(sduTests ${GTEST_MAIN_LIBRARY} -L "${CMAKE_BINARY_DIR}/libs" testhelpers remotediagnoseimpl)

gtest_discover_tests(sduTests)

SET_TARGET_PROPERTIES(sduTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")