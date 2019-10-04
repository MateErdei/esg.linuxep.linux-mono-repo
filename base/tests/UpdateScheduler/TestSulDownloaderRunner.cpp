/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <UpdateSchedulerImpl/Logger.h>
#include <UpdateSchedulerImpl/runnerModule/SulDownloaderRunner.h>
#include <gmock/gmock-matchers.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/ProcessImpl/MockProcess.h>
#include <tests/watchdog/watchdogimpl/MockIWatchdogRequest.h>

#include <future>

using namespace UpdateSchedulerImpl;
using namespace UpdateScheduler;
using SulDownloaderRunner = UpdateSchedulerImpl::runnerModule::SulDownloaderRunner;

class TestSulDownloaderRunner : public ::testing::Test
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

    void setupMockForCheckServiceFailure()
    {
        Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
            auto mockProcess1 = new StrictMock<MockProcess>();
            std::vector<std::string> args{ "is-failed", "sophos-spl-update.service" };
            EXPECT_CALL(*mockProcess1, exec("/bin/systemctl", args)).Times(1);
            EXPECT_CALL(*mockProcess1, output()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess1, exitCode()).WillOnce(Return(0));

            return std::unique_ptr<Common::Process::IProcess>(mockProcess1);
        });
    }

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestSulDownloaderRunner, SuccessfulRun) // NOLINT
{
    // Create temp directory to use for the update_report.json file.
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();

    // Mock systemctl call.
    setupMockForCheckServiceSuccess();

    IWatchdogRequestReplacement replacement;

    // Create task queue.
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    SulDownloaderRunner runner(queue, tempDir->dirPath(), "update_report.json", std::chrono::seconds(5));
    auto futureRunner = std::async(std::launch::async, [&runner]() { runner.run(); });

    // Write a report json file.
    auto futureTempDir = std::async(
        std::launch::async, [&tempDir]() { tempDir->createFileAtomically("update_report.json", "some json"); });

    // Check result from suldownloader runner, NB queue will block until item available.
    auto task = queue->pop();
    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderFinished);
    EXPECT_EQ(task.content, "update_report.json");
}

TEST_F(TestSulDownloaderRunner, SulDownloaderRunsThenFails) // NOLINT
{
    // Create temp directory to use for the update_report.json file.
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();

    // Mock systemctl call.
    setupMockForCheckServiceFailure();
    IWatchdogRequestReplacement replacement;

    testing::internal::CaptureStderr();

    // Create task queue.
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    SulDownloaderRunner runner(queue, tempDir->dirPath(), "update_report.json", std::chrono::seconds(5));
    auto futureRunner = std::async(std::launch::async, [&runner]() { runner.run(); });

    // Write a report json file.
    auto futureTempDir = std::async(
        std::launch::async, [&tempDir]() { tempDir->createFileAtomically("update_report.json", "some json"); });

    // Check result from suldownloader runner, NB queue will block until item available.
    auto task = queue->pop();
    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderFinished);
    EXPECT_EQ(task.content, "update_report.json");

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Update Service (sophos-spl-update.service) failed."));
}

TEST_F(TestSulDownloaderRunner, SuccessfulRunWithWait) // NOLINT
{
    // Create temp directory to use for the update_report.json file.
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();

    setupMockForCheckServiceSuccess();
    IWatchdogRequestReplacement replacement;

    // Create task queue.
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    SulDownloaderRunner runner(queue, tempDir->dirPath(), "update_report.json", std::chrono::seconds(3));
    std::thread runnerThread([&runner]() { runner.run(); });

    auto fut = std::async(std::launch::async, [&tempDir]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        tempDir->createFileAtomically("update_report.json", "some json");
    });

    runnerThread.join();

    auto task = queue->pop();
    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderFinished);
    EXPECT_EQ(task.content, "update_report.json");
}

TEST_F(TestSulDownloaderRunner, Timeout) // NOLINT
{
    IWatchdogRequestReplacement replacement;
    // Create task queue
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner which will timeout after 1 second of waiting and run it.
    SulDownloaderRunner runner(queue, "/tmp", "some-string", std::chrono::seconds(1));
    std::thread runnerThread([&runner]() { runner.run(); });

    runnerThread.join();

    auto task = queue->pop();
    ASSERT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderTimedOut);
}

TEST_F(TestSulDownloaderRunner, Aborted) // NOLINT
{
    IWatchdogRequestReplacement replacement;

    // Create task queue
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    SulDownloaderRunner runner(queue, "/tmp", "update_report.json", std::chrono::seconds(10));
    std::thread runnerThread([&runner]() { runner.run(); });

    runner.abortWaitingForReport();
    runnerThread.join();
    auto task = queue->pop();
    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderMonitorDetached);
}

TEST_F(TestSulDownloaderRunner, FailedToStart) // NOLINT
{
    std::string errorMessage = "bad thing happened";
    IWatchdogRequestReplacement replacement(errorMessage);

    // Create task queue
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    SulDownloaderRunner runner(queue, "/tmp", "update_report.json", std::chrono::seconds(10));
    std::thread runnerThread([&runner]() { runner.run(); });

    runner.abortWaitingForReport();
    runnerThread.join();
    auto task = queue->pop();
    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderFailedToStart);
    EXPECT_EQ(task.content, errorMessage);
}