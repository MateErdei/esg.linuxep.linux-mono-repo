include(GoogleTest)
add_executable(TestManagementAgentImpl
        ManagementAgentImplTests.cpp
        ManagementAgentIntegrationTests.cpp
        ../Common/TaskQueueImpl/FakeQueue.h
        )
target_include_directories(TestManagementAgentImpl SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_include_directories(TestManagementAgentImpl BEFORE PUBLIC  ${CMAKE_SOURCE_DIR})
target_link_libraries(TestManagementAgentImpl ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY})
target_link_libraries(TestManagementAgentImpl managementagentimpl testhelpers updateschedulerimpl pluginapiimpl)
gtest_discover_tests(TestManagementAgentImpl)

SET_TARGET_PROPERTIES(TestManagementAgentImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )