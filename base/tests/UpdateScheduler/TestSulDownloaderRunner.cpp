/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <UpdateScheduler/SulDownloaderRunner.h>
#include <gmock/gmock-matchers.h>
#include <modules/Common/ProcessImpl/ProcessImpl.h>
#include <tests/Common/ProcessImpl/MockProcess.h>
#include <tests/Common/TestHelpers/TempDir.h>
#include <future>

using namespace UpdateScheduler;

class TestSulDownloaderRunner : public ::testing::Test
{

public:

    void SetUp() override
    {
    }

    void TearDown() override
    {
        Common::ProcessImpl::ProcessFactory::instance().restoreCreator();
    }

    MockProcess* setupMockProcess()
    {
        auto mockProcess = new MockProcess();
        Common::ProcessImpl::ProcessFactory::instance().replaceCreator([mockProcess]() {
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });
        return mockProcess;
    }
};

TEST_F(TestSulDownloaderRunner, SuccessfulRun) // NOLINT
{
    // Create temp directory to use for the report.json file.
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();

    // Mock systemctl call.
    MockProcess * mockProcess = setupMockProcess();
    EXPECT_CALL(*mockProcess, exec(_,_)).Times(1);
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));

    // Create task queue.
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    SulDownloaderRunner runner(queue, tempDir->dirPath(), "report.json", std::chrono::seconds(5));
    std::thread runnerThread([&runner]() {
        runner.run();
    });

    // Write a report json file.
    auto fut = std::async(std::launch::async, [&tempDir]() {
        tempDir->createFileAtomically("report.json", "some json");
    });
    runnerThread.join();

    // Check result from suldownloader runner.
    auto task = queue->pop();
    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderFinished);
    EXPECT_EQ(task.content, "report.json");
}

TEST_F(TestSulDownloaderRunner, SuccessfulRunWithWait) // NOLINT
{
    // Create temp directory to use for the report.json file.
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();

    // Mock systemctl call.
    MockProcess * mockProcess = setupMockProcess();
    EXPECT_CALL(*mockProcess, exec(_,_)).Times(1);
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));

    // Create task queue.
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    SulDownloaderRunner runner(queue, tempDir->dirPath(), "report.json", std::chrono::seconds(3));
    std::thread runnerThread([&runner]() {
        runner.run();
    });

    auto fut = std::async(std::launch::async,  [&tempDir]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); tempDir->createFileAtomically("report.json", "some json");
    });

    runnerThread.join();

    auto task = queue->pop();
    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderFinished);
    EXPECT_EQ(task.content, "report.json");
}


TEST_F(TestSulDownloaderRunner, Timeout) // NOLINT
{
    // Mock systemctl call.
    MockProcess * mockProcess = setupMockProcess();
    EXPECT_CALL(*mockProcess, exec(_,_)).Times(1);
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));

    // Create task queue
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner which will timeout after 1 second of waiting and run it.
    SulDownloaderRunner runner(queue, "/tmp", "some-string", std::chrono::seconds(1));
    std::thread runnerThread([&runner]() {
        runner.run();
    });

    runnerThread.join();

    auto task = queue->pop();
    ASSERT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderTimedOut);
}

TEST_F(TestSulDownloaderRunner, Abort) // NOLINT
{
    // Mock systemctl call
    MockProcess * mockProcess = setupMockProcess();
    EXPECT_CALL(*mockProcess, exec(_,_)).Times(1);
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));

    // Create task queue
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    // Create suldownloader runner and run it.
    SulDownloaderRunner runner(queue, "/tmp", "report.json", std::chrono::seconds(10));
    std::thread runnerThread([&runner]() {
        runner.run();
    });

    runner.abort();
    runnerThread.join();
    auto task = queue->pop();
    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderWasAborted);
}