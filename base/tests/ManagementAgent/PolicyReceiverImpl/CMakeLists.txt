include(GoogleTest)
add_executable(PolicyReceiverImplTests
        PolicyReceiverImplTests.cpp
        ../../Common/TaskQueueImpl/FakeQueue.h
        )
target_include_directories(PolicyReceiverImplTests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_include_directories(PolicyReceiverImplTests BEFORE PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(PolicyReceiverImplTests  ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${})
target_link_libraries(PolicyReceiverImplTests policyreceiverimpl testhelpers )

gtest_discover_tests(PolicyReceiverImplTests)
SET_TARGET_PROPERTIES(PolicyReceiverImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )