/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ProcessImpl/ProcessImpl.h>
#include <UpdateSchedulerImpl/Logger.h>
#include <UpdateSchedulerImpl/runnerModule/SulDownloaderRunner.h>
#include <gmock/gmock-matchers.h>
#include <tests/Common/Helpers/TempDir.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <tests/Common/ProcessImpl/MockProcess.h>

#include <future>

using namespace UpdateSchedulerImpl;
using namespace UpdateScheduler;
using SulDownloaderRunner = UpdateSchedulerImpl::runnerModule::SulDownloaderRunner;
class TestSulDownloaderRunner : public ::testing::Test
{
public:

    void TearDown() override { Common::ProcessImpl::ProcessFactory::instance().restoreCreator(); }

    MockProcess* setupMockProcess()
    {
        auto mockProcess = new MockProcess();
        Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            [mockProcess]() { return std::unique_ptr<Common::Process::IProcess>(mockProcess); });
        return mockProcess;
    }

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestSulDownloaderRunner, SuccessfulRun) // NOLINT
{
    // Create temp directory to use for the report.json file.
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();

    // Mock systemctl call.
    MockProcess* mockProcess = setupMockProcess();
    EXPECT_CALL(*mockProcess, exec(_, _)).Times(1);
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));

    // Create task queue.
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    SulDownloaderRunner runner(queue, tempDir->dirPath(), "report.json", std::chrono::seconds(5));
    auto futureRunner = std::async(std::launch::async, [&runner]() { runner.run(); });

    // Write a report json file.
    auto futureTempDir =
        std::async(std::launch::async, [&tempDir]() { tempDir->createFileAtomically("report.json", "some json"); });

    // Check result from suldownloader runner, NB queue will block until item available.
    auto task = queue->pop();
    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderFinished);
    EXPECT_EQ(task.content, "report.json");
}

TEST_F(TestSulDownloaderRunner, SuccessfulRunWithWait) // NOLINT
{
    // Create temp directory to use for the report.json file.
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();

    // Mock systemctl call.
    MockProcess* mockProcess = setupMockProcess();
    EXPECT_CALL(*mockProcess, exec(_, _)).Times(1);
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));

    // Create task queue.
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    SulDownloaderRunner runner(queue, tempDir->dirPath(), "report.json", std::chrono::seconds(3));
    std::thread runnerThread([&runner]() { runner.run(); });

    auto fut = std::async(std::launch::async, [&tempDir]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        tempDir->createFileAtomically("report.json", "some json");
    });

    runnerThread.join();

    auto task = queue->pop();
    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderFinished);
    EXPECT_EQ(task.content, "report.json");
}

TEST_F(TestSulDownloaderRunner, Timeout) // NOLINT
{
    // Mock systemctl call.
    MockProcess* mockProcess = setupMockProcess();
    EXPECT_CALL(*mockProcess, exec(_, _)).Times(1);
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));

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
    // Mock systemctl call
    MockProcess* mockProcess = setupMockProcess();
    EXPECT_CALL(*mockProcess, exec(_, _)).Times(1);
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));

    // Create task queue
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    SulDownloaderRunner runner(queue, "/tmp", "report.json", std::chrono::seconds(10));
    std::thread runnerThread([&runner]() { runner.run(); });

    runner.abortWaitingForReport();
    runnerThread.join();
    auto task = queue->pop();
    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderMonitorDetached);
}

TEST_F(TestSulDownloaderRunner, FailedToStart) // NOLINT
{
    std::string errorMessage = "bad thing happened";

    // Mock systemctl call
    MockProcess* mockProcess = setupMockProcess();
    EXPECT_CALL(*mockProcess, exec(_, _)).Times(1);
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(errorMessage));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));

    // Create task queue
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    SulDownloaderRunner runner(queue, "/tmp", "report.json", std::chrono::seconds(10));
    std::thread runnerThread([&runner]() { runner.run(); });

    runner.abortWaitingForReport();
    runnerThread.join();
    auto task = queue->pop();
    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderFailedToStart);
    EXPECT_EQ(task.content, errorMessage);
}