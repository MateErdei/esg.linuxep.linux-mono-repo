include(GoogleTest)
add_executable(TestZMQWrapperApiImpl
        TestContextImpl.cpp
        TestLoggingProxyImpl.cpp)

target_compile_definitions(TestZMQWrapperApiImpl PUBLIC CMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(TestZMQWrapperApiImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ${ZeroMQ_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
target_include_directories(TestZMQWrapperApiImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(TestZMQWrapperApiImpl gtest gtest_main)
target_link_libraries(TestZMQWrapperApiImpl zmqwrapperapiimpl threads testhelpers)
gtest_discover_tests(TestZMQWrapperApiImpl)
