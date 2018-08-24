include(GoogleTest)
add_executable(TestEventReceiverImpl
        TestEventReceiverImpl.cpp
        TestEventTask.cpp)
target_include_directories(TestEventReceiverImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestEventReceiverImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ../../../)
target_link_libraries(TestEventReceiverImpl gtest gtest_main gmock)
target_link_libraries(TestEventReceiverImpl eventreceiverimpl testhelpers)
gtest_discover_tests(TestEventReceiverImpl)
