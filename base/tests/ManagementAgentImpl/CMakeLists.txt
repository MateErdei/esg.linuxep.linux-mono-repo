include(GoogleTest)
add_executable(TestManagementAgentImpl
        ManagementAgentImplTests.cpp
        ../Common/TaskQueueImpl/FakeQueue.h
        )
target_include_directories(TestManagementAgentImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestManagementAgentImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ../../)
target_link_libraries(TestManagementAgentImpl gtest gtest_main gmock)
target_link_libraries(TestManagementAgentImpl managementagentimpl)
gtest_discover_tests(TestManagementAgentImpl)

