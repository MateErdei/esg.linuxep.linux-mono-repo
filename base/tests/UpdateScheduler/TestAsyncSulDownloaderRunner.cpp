/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <UpdateScheduler/IAsyncSulDownloaderRunner.h>
#include <UpdateSchedulerImpl/Logger.h>
#include <gmock/gmock-matchers.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/ProcessImpl/MockProcess.h>
#include <tests/watchdog/watchdogimpl/MockIWatchdogRequest.h>

#include <future>

using namespace UpdateScheduler;

class TestAsyncSulDownloaderRunner : public ::testing::Test
{
public:
    void TearDown() override { Common::ProcessImpl::ProcessFactory::instance().restoreCreator(); }

    void setupMockForCheckServiceSuccess()
    {
        Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
            auto mockProcess1 = new StrictMock<MockProcess>();
            std::vector<std::string> args{ "is-failed", "sophos-spl-update.service" };
            EXPECT_CALL(*mockProcess1, exec("/bin/systemctl", args)).Times(1);
            EXPECT_CALL(*mockProcess1, output()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess1, exitCode()).WillOnce(Return(1));

            return std::unique_ptr<Common::Process::IProcess>(mockProcess1);
        });
    }

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestAsyncSulDownloaderRunner, triggerSulDownloader) // NOLINT
{
    // Create temp directory to use for the update_report.json file.
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();

    setupMockForCheckServiceSuccess();
    IWatchdogRequestReplacement replacement;

    // Create task queue.
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    std::unique_ptr<IAsyncSulDownloaderRunner> runner = createSulDownloaderRunner(queue, tempDir->dirPath());
    runner->triggerSulDownloader();

    // Write a report json file.
    auto futureTempDir = std::async(
        std::launch::async, [&tempDir]() { tempDir->createFileAtomically("update_report.json", "some json"); });

    // Check result from suldownloader runner, NB queue will block until item available.
    auto task = queue->pop();

    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderFinished);
    EXPECT_EQ(task.content, "update_report.json");
    if (runner->isRunning())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        EXPECT_FALSE(runner->isRunning());
    }
}

TEST_F(TestAsyncSulDownloaderRunner, isRunningAndAbort) // NOLINT
{
    // Create temp directory to use for the update_report.json file.
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();

    IWatchdogRequestReplacement replacement([]() {
        MockIWatchdogRequest* mock = new StrictMock<MockIWatchdogRequest>();
        EXPECT_CALL(*mock, requestUpdateService).WillOnce(Invoke([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }));
        return std::unique_ptr<watchdog::watchdogimpl::IWatchdogRequest>(mock);
    });

    // Create task queue.
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    std::unique_ptr<IAsyncSulDownloaderRunner> runner = createSulDownloaderRunner(queue, tempDir->dirPath());
    runner->triggerSulDownloader();

    EXPECT_TRUE(runner->isRunning());

    runner->triggerAbort();

    // Check result from suldownloader runner, NB queue will block until item available.
    auto task = queue->pop();

    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderMonitorDetached);
}
