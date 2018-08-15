/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <UpdateScheduler/SulDownloaderRunner.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <modules/Common/ProcessImpl/ProcessImpl.h>
#include <tests/Common/ProcessImpl/MockProcess.h>
#include <modules/Common/FileSystem/IFileSystem.h>
#include <modules/Common/FileSystemImpl/FileSystemImpl.h>
#include <tests/Common/TestHelpers/TempDir.h>
#include "Common/FileSystem/IFileSystem.h"

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


TEST_F(TestSulDownloaderRunner, Timeout) // NOLINT
{
    MockProcess * mockProcess = setupMockProcess();

    // systemctl call
    EXPECT_CALL(*mockProcess, exec(_,_)).Times(1);
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));

    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());
    SulDownloaderRunner runner(queue, "/tmp");
    runner.run(std::chrono::seconds(1));
    auto task = queue->pop();

    ASSERT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderTimedOut);
    //std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());
    //CronSchedulerThread schedulerThread(queue, milliseconds(10), milliseconds(20) );
}

//class FileSystemImplTest : public ::testing::Test
//{
//public:
//    std::unique_ptr<IFileSystem> m_fileSystem;
//    void SetUp() override
//    {
//        m_fileSystem.reset(new FileSystemImpl());
//    }
//};

TEST_F(TestSulDownloaderRunner, SuccessfulRun)
{


//    ////////////////////
//    using namespace Common::FileSystem;
//    std::unique_ptr<IFileSystem> fakeFileSystem = std::unique_ptr<IFileSystem>(new FileSystemImpl());
//
//    std::string filePath = Common::FileSystem::join(fakeFileSystem->currentWorkingDirectory(), fileName);
//
//    //std::string testContent("HelloWorld");
//
//    fakeFileSystem->writeFileAtomically(filePath, "Some content", fakeFileSystem->currentWorkingDirectory() );
//
//    //EXPECT_EQ(m_fileSystem->readFile(filePath), testContent);
//    ::remove(filePath.c_str());
//    //////////////////


    std::string fileName = "report.json";
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();
    MockProcess * mockProcess = setupMockProcess();

    // mock systemctl call
    EXPECT_CALL(*mockProcess, exec(_,_)).Times(1);
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());
    SulDownloaderRunner runner(queue, tempDir->dirPath());
    std::thread runnerThread(runner.run, 1);
    tempDir->createFile(fileName, "Some json");

    runnerThread.join();

    auto task = queue->pop();
    ASSERT_EQ(task.taskType, SchedulerTask::TaskType::SulDownloaderTimedOut);
    //std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());
    //CronSchedulerThread schedulerThread(queue, milliseconds(10), milliseconds(20) );


}
