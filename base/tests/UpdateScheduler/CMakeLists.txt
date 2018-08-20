include(GoogleTest)
add_executable(UpdateSchedulerTests
        TestUpdatePolicyTranslator.cpp
        TestUpdateActionParser.cpp
        TestCronSchedulerThread.cpp
        TestSulDownloaderRunner.cpp)

target_include_directories(UpdateSchedulerTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(UpdateSchedulerTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}")
target_link_libraries(UpdateSchedulerTests gtest gmock gtest_main)
target_link_libraries(UpdateSchedulerTests updateschedulerimpl filesystemimpl testhelpers)
gtest_discover_tests(UpdateSchedulerTests)