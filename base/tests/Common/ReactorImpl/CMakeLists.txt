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
target_include_directories(ReactorImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ${ZeroMQ_INCLUDE_DIR})
target_include_directories(ReactorImplTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" ../../../)
target_link_libraries(ReactorImplTests gtest gmock)
target_link_libraries(ReactorImplTests reactorimpl threads testhelpers processimpl)
gtest_discover_tests(ReactorImplTests)

add_executable(FakeServerRunner FakeServer.cpp FakeServer.h TestListener.h TestListener.cpp FakeServerRunner.cpp)
target_link_libraries(FakeServerRunner PUBLIC reactorimpl zeromqwrapperimpl threads)
add_dependencies(ReactorImplTests FakeServerRunner)



