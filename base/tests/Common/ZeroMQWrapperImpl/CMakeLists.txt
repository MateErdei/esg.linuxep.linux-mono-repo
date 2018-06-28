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
        )

target_include_directories(TestZeroMQWrapperImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include")
target_include_directories(TestZeroMQWrapperImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(TestZeroMQWrapperImpl gtest gtest_main)
target_link_libraries(TestZeroMQWrapperImpl zeromqwrapperimpl)
gtest_discover_tests(TestZeroMQWrapperImpl)
