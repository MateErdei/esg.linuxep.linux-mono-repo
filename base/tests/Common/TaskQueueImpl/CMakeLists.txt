include(GoogleTest)
add_executable(TestTaskQueueImpl
        TestTaskQueueImpl.cpp
        )
target_include_directories(TestTaskQueueImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestTaskQueueImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(TestTaskQueueImpl gtest gtest_main)
target_link_libraries(TestTaskQueueImpl taskqueueimpl threads)
gtest_discover_tests(TestTaskQueueImpl)
