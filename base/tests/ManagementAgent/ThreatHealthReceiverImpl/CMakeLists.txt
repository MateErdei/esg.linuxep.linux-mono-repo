include(GoogleTest)
add_executable(TestThreatHealthReceiverImpl
        TestThreatHealthReceiverImpl.cpp
        TestThreatHealthTask.cpp
        ../../Common/TaskQueueImpl/FakeQueue.h
        )
target_include_directories(TestThreatHealthReceiverImpl SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_include_directories(TestThreatHealthReceiverImpl BEFORE PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(TestThreatHealthReceiverImpl ${GTEST_MAIN_LIBRARY} ThreatHealthReceiverImpl testhelpers HealthStatusImpl)

gtest_discover_tests(TestThreatHealthReceiverImpl)
SET_TARGET_PROPERTIES(TestThreatHealthReceiverImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )