include(GoogleTest)
add_executable(TestStatusReceiverImpl
        TestStatusReceiverImpl.cpp
        TestStatusTask.cpp
        ../../Common/TaskQueueImpl/FakeQueue.h
        )
target_include_directories(TestStatusReceiverImpl SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_include_directories(TestStatusReceiverImpl BEFORE PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(TestStatusReceiverImpl ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
target_link_libraries(TestStatusReceiverImpl statusreceiverimpl testhelpers )

gtest_discover_tests(TestStatusReceiverImpl)
SET_TARGET_PROPERTIES(TestStatusReceiverImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )