include(GoogleTest)
add_executable(TestStatusReceiverImpl
        TestStatusReceiverImpl.cpp
        TestStatusTask.cpp
        ../../Common/TaskQueueImpl/FakeQueue.h
        ../../Common/Logging/TestConsoleLoggingSetup.h
        ../../Common/Logging/TestConsoleLoggingSetup.cpp
        )
target_include_directories(TestStatusReceiverImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestStatusReceiverImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_link_libraries(TestStatusReceiverImpl gtest gtest_main gmock)
target_link_libraries(TestStatusReceiverImpl statusreceiverimpl testhelpers )

gtest_discover_tests(TestStatusReceiverImpl)
