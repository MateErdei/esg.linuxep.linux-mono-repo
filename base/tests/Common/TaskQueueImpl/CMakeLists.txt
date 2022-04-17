include(GoogleTest)
add_executable(TestTaskQueueImpl
        TestTaskQueueImpl.cpp
        TestTaskProcessorImpl.cpp)
target_include_directories(TestTaskQueueImpl SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
#target_include_directories(TestTaskQueueImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(TestTaskQueueImpl ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY})
target_link_libraries(TestTaskQueueImpl taskqueueimpl threads)
gtest_discover_tests(TestTaskQueueImpl)
SET_TARGET_PROPERTIES(TestTaskQueueImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )