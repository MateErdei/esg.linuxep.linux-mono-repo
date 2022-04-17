include(GoogleTest)
add_executable(TestZMQWrapperApiImpl
        TestContextImpl.cpp
        TestLoggingProxyImpl.cpp)

target_compile_definitions(TestZMQWrapperApiImpl PUBLIC CMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(TestZMQWrapperApiImpl SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" ${ZeroMQ_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
#target_include_directories(TestZMQWrapperApiImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(TestZMQWrapperApiImpl ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY})
target_link_libraries(TestZMQWrapperApiImpl zmqwrapperapiimpl threads testhelpers)
gtest_discover_tests(TestZMQWrapperApiImpl)
SET_TARGET_PROPERTIES(TestZMQWrapperApiImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )