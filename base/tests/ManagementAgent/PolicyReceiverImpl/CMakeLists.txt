include(GoogleTest)
add_executable(PolicyReceiverImplTests
        PolicyReceiverImplTests.cpp
        ../../Common/TaskQueueImpl/FakeQueue.h)
target_include_directories(PolicyReceiverImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(PolicyReceiverImplTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ../../../)
target_link_libraries(PolicyReceiverImplTests gtest gtest_main gmock)
target_link_libraries(PolicyReceiverImplTests policyreceiverimpl pluginapiimpl taskqueueimpl plugincommunicationimpl mcsrouterplugincommunicationimpl testhelpers )
gtest_discover_tests(PolicyReceiverImplTests)