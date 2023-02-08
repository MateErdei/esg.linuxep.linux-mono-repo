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
        MockCronSchedulerThread.h
        TestWithMockTimerAbstract.h
        TestDownloadStateMachine.cpp
        TimeStamp.h
        TimeStamp.cpp
        Macros.h
        InstallStateMachineTests.cpp
        EventStateMachineTests.cpp
        TestStateMachineData.cpp
        StateMachineDataBase.h
        TestStateMachineProcessor.cpp
        TestUpdateSchedulerPluginUtils.cpp)

target_include_directories(UpdateSchedulerTests SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE} ${LOG4CPLUS_INCLUDE_DIR} "${PROJECT_SOURCE_DIR}")
target_link_libraries(UpdateSchedulerTests ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} updateschedulerimpl filesystemimpl testhelpers processimpl suldownloaderdata)

SET_TARGET_PROPERTIES(UpdateSchedulerTests
        PROPERTIES
        INSTALL_RPATH "${SUL_LIBRARY_DIRS}"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

gtest_discover_tests(UpdateSchedulerTests)