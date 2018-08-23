include(GoogleTest)

add_executable(UpdateSchedulerTests
        TestUpdatePolicyTranslator.cpp
        TestUpdateActionParser.cpp
        TestCronSchedulerThread.cpp
        TestDownloadReportsAnalyser.cpp
        TestUpdateEvent.cpp
        DownloadReportTestBuilder.h
        MockMapHostCacheId.h
        TestUpdateStatus.cpp
        TestSulDownloaderRunner.cpp
        TestAsyncSulDownloaderRunner.cpp
        TestUpdateScheduler.cpp
        MockAsyncDownloaderRunner.h
        MockCronSchedulerThread.h)

target_include_directories(UpdateSchedulerTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ${LOG4CPLUS_INCLUDE_DIR} "${PROJECT_SOURCE_DIR}")
target_link_libraries(UpdateSchedulerTests gtest gmock gtest_main updateschedulerimpl filesystemimpl testhelpers processimpl suldownloaderdata)


gtest_discover_tests(UpdateSchedulerTests)