include(GoogleTest)
add_executable(TestStatusReceiverImpl
        TestStatusCache.cpp
        TestStatusReceiverImpl.cpp
        TestStatusTask.cpp
        ../../Common/TaskQueueImpl/FakeQueue.h)
target_include_directories(TestStatusReceiverImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestStatusReceiverImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ../../../)
target_link_libraries(TestStatusReceiverImpl gtest gtest_main gmock)
target_link_libraries(TestStatusReceiverImpl statusreceiverimpl taskqueueimpl testhelpers statuscacheimpl)
gtest_discover_tests(TestStatusReceiverImpl)
