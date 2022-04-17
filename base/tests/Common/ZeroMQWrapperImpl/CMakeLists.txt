include(GoogleTest)
add_executable(TestZeroMQWrapperImpl
        TestZeroMQWrapperImpl.cpp
        TestContextHolder.cpp
        TestSocketHolder.cpp
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

SET_TARGET_PROPERTIES(TestZeroMQWrapperImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

target_compile_definitions(TestZeroMQWrapperImpl PUBLIC CMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(TestZeroMQWrapperImpl SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" ${ZeroMQ_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
#target_include_directories(TestZeroMQWrapperImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(TestZeroMQWrapperImpl ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY})
target_link_libraries(TestZeroMQWrapperImpl zmqwrapperapiimpl processimpl threads testhelpers)
gtest_discover_tests(TestZeroMQWrapperImpl)

add_executable(TestReqRepTool
        ReqRepTool.cpp
        ReqRepTestImplementations.cpp
        ReqRepTestImplementations.h)


target_include_directories(TestReqRepTool SYSTEM BEFORE PUBLIC ${ZeroMQ_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(TestReqRepTool zeromqwrapperimpl threads testhelpers)

SET_TARGET_PROPERTIES(TestReqRepTool
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
