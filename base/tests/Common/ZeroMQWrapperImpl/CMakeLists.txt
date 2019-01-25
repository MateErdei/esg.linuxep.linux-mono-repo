include(GoogleTest)
add_executable(TestZeroMQWrapperImpl
        TestZeroMQWrapperImpl.cpp
        TestContextHolder.cpp
        TestSocketHolder.cpp
        TestContextImpl.cpp
        TestPollerImpl.cpp
        TestProxyImpl.cpp
        TestSocketRequesterImpl.cpp
        TestSocketReplierImpl.cpp
        TestSocketPublisherImpl.cpp
        TestSocketSubscriberImpl.cpp
        TestReqRepReliability.cpp
        TestPubSubReliability.cpp
        ReqRepTestImplementations.cpp
        ReqRepTestImplementations.h)

target_compile_definitions(TestZeroMQWrapperImpl PUBLIC CMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(TestZeroMQWrapperImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ${ZeroMQ_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
target_include_directories(TestZeroMQWrapperImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(TestZeroMQWrapperImpl gtest gtest_main)
target_link_libraries(TestZeroMQWrapperImpl zeromqwrapperimpl threads testhelpers)
gtest_discover_tests(TestZeroMQWrapperImpl)

add_executable(TestReqRepTool
        ReqRepTool.cpp
        ReqRepTestImplementations.cpp
        ReqRepTestImplementations.h)


target_include_directories(TestReqRepTool SYSTEM BEFORE PUBLIC ${ZeroMQ_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(TestReqRepTool zeromqwrapperimpl threads testhelpers)
