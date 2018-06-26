add_executable(ReactorImplTests
                ReactorImplTests.cpp
                MockCallBackListener.h
                MockShutdownListener.h
                PipeForTests.cpp
                PipeForTests.h
                FakeServer.cpp
                FakeServer.h TestListener.cpp TestListener.h FakeClient.cpp FakeClient.h TestGenericShutdownListener.cpp TestGenericCallbackListener.cpp)
target_include_directories(ReactorImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(ReactorImplTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" ../TestHelpers)
target_link_libraries(ReactorImplTests gtest gtest_main gmock)
target_link_libraries(ReactorImplTests reactorimpl threads testhelpers processimpl)
add_test(NAME ReactorImplTests COMMAND ReactorImplTests)

add_executable(FakeServerRunner FakeServer.cpp FakeServer.h TestListener.h TestListener.cpp FakeServerRunner.cpp)
target_link_libraries(FakeServerRunner PUBLIC reactorimpl zeromqwrapperimpl threads)
add_dependencies(ReactorImplTests FakeServerRunner)



