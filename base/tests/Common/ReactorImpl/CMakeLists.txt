include(GoogleTest)
add_executable(ReactorImplTests
                ReactorImplTests.cpp
                MockCallBackListener.h
                MockShutdownListener.h
                PipeForTests.cpp
                PipeForTests.h
                FakeServer.cpp
        FakeServer.h
        TestListener.cpp
        TestListener.h
        FakeClient.h
        FakeClient.cpp
        TestGenericShutdownListener.cpp
        TestGenericCallbackListener.cpp
        ReactorImplTestsPath.h
        ReactorImplTestsPath.cpp
        testrunner_main.cpp)
target_include_directories(ReactorImplTests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE} ${ZeroMQ_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(ReactorImplTests ${GTEST_MAIN_LIBRARY})
target_link_libraries(ReactorImplTests threads testhelpers processimpl zmqwrapperapiimpl)
gtest_discover_tests(ReactorImplTests)

add_executable(FakeServerRunner FakeServer.cpp FakeServer.h TestListener.h TestListener.cpp FakeServerRunner.cpp)
target_link_libraries(FakeServerRunner PUBLIC zmqwrapperapiimpl threads)
add_dependencies(ReactorImplTests FakeServerRunner)
SET_TARGET_PROPERTIES(ReactorImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )


