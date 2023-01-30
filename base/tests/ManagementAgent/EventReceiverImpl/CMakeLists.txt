include(GoogleTest)
add_executable(TestEventReceiverImpl
        TestEventReceiverImpl.cpp
        TestEventTask.cpp
        TestOutbreakModeController.cpp)

target_include_directories(TestEventReceiverImpl SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}")
target_include_directories(TestEventReceiverImpl BEFORE PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(TestEventReceiverImpl ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY})
target_link_libraries(TestEventReceiverImpl eventreceiverimpl testhelpers)
gtest_discover_tests(TestEventReceiverImpl)
SET_TARGET_PROPERTIES(TestEventReceiverImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )