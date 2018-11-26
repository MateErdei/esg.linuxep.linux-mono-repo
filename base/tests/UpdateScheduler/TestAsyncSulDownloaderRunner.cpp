/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <UpdateScheduler/IAsyncSulDownloaderRunner.h>
#include <UpdateSchedulerImpl/Logger.h>

#include <Common/ProcessImpl/ProcessImpl.h>

#include <tests/Common/Logging/TestConsoleLoggingSetup.h>
#include <tests/Common/ProcessImpl/MockProcess.h>
#include <tests/Common/TestHelpers/TempDir.h>

#include <gmock/gmock-matchers.h>

#include <future>

using namespace UpdateScheduler;

class TestAsyncSulDownloaderRunner
        : public ::testing::Test
{

public:

    void SetUp() override
    {
        m_loggingSetup.reset(new TestLogging::TestConsoleLoggingSetup());
    }

    void TearDown() override
    {
        Common::ProcessImpl::ProcessFactory::instance().restoreCreator();
        m_loggingSetup.reset();
    }

    MockProcess* setupMockProcess()
    {
        auto mockProcess = new MockProcess();
        Common::ProcessImpl::ProcessFactory::instance().replaceCreator([mockProcess]() {
                                                                           return std::unique_ptr<Common::Process::IProcess>(mockProcess);
                                                                       }
        );
        return mockProcess;
    }

    TestLogging::TestConsoleLoggingSetupPtr m_loggingSetup;
};

TEST_F(TestAsyncSulDownloaderRunner, triggerSulDownloader) // NOLINT
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
    std::unique_ptr<IAsyncSulDownloaderRunner> runner = createSulDownloaderRunner(queue, tempDir->dirPath());
    runner->triggerSulDownloader();

    // Write a report json file.
    auto futureTempDir = std::async(std::launch::async, [&tempDir]() {
                                        tempDir->createFileAtomically("report.json", "some json");
                                    }
    );

    // Check result from suldownloader runner, NB queue will block until item available.
    auto task = queue->pop();

    EXPECT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderFinished);
    EXPECT_EQ(task.content, "report.json");
    if (runner->isRunning())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        EXPECT_FALSE(runner->isRunning());
    }
}


TEST_F(TestAsyncSulDownloaderRunner, isRunningAndAbort) // NOLINT
{

    // Create temp directory to use for the report.json file.
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();

    // Mock systemctl call.
    MockProcess* mockProcess = setupMockProcess();
    EXPECT_CALL(*mockProcess, exec(_, _)).Times(1);
    EXPECT_CALL(*mockProcess, output()).WillOnce(Invoke(
            []() {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                return "";
            }
    ));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));

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

