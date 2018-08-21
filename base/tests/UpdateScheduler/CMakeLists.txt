include(GoogleTest)

set (UpdateSchedulerTests_RPATH  ${SUL_LIBRARY_DIRS})

add_executable(UpdateSchedulerTests
        TestUpdatePolicyTranslator.cpp
        TestUpdateActionParser.cpp
        TestCronSchedulerThread.cpp
        TestDownloadReportsAnalyser.cpp
        TestUpdateEvent.cpp
        DownloadReportTestBuilder.h
        MockMapHostCacheId.h
        TestUpdateStatus.cpp
        TestSulDownloaderRunner.cpp)

target_include_directories(UpdateSchedulerTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(UpdateSchedulerTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}")
target_link_libraries(UpdateSchedulerTests gtest gmock gtest_main updatescheduler filesystemimpl testhelpers )

SET_TARGET_PROPERTIES(UpdateSchedulerTests
        PROPERTIES
        INSTALL_RPATH "${UpdateSchedulerTests_RPATH}"
        BUILD_RPATH   "${UpdateSchedulerTests_RPATH}"
        )

gtest_discover_tests(UpdateSchedulerTests)