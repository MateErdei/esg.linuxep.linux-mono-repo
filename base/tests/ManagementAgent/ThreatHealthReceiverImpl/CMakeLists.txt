include(GoogleTest)
add_executable(TestThreatHealthReceiverImpl
        TestThreatHealthReceiverImpl.cpp
        TestThreatHealthTask.cpp
        ../../Common/TaskQueueImpl/FakeQueue.h
        )
target_include_directories(TestThreatHealthReceiverImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestThreatHealthReceiverImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_link_libraries(TestThreatHealthReceiverImpl gtest gtest_main gmock ThreatHealthReceiverImpl testhelpers HealthStatusImpl)

gtest_discover_tests(TestThreatHealthReceiverImpl)
