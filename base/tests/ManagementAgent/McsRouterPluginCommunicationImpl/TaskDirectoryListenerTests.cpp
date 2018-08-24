/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>
#include <Common/TaskQueueImpl/TaskProcessorImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/TaskDirectoryListener.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <modules/Common/FileSystemImpl/FileSystemImpl.h>
#include <modules/ManagementAgent/LoggerImpl/LoggingSetup.h>

#include "MockPluginManager.h"
#include "MockTaskQueue.h"

class TaskDirectoryListenerTests : public ::testing::Test
{
public:
    TaskDirectoryListenerTests()
    : m_loggingSetup(std::unique_ptr<ManagementAgent::LoggerImpl::LoggingSetup>(new ManagementAgent::LoggerImpl::LoggingSetup(1)))
    {

    }

    void SetUp() override
    {
        m_mockTaskQueue = std::make_shared<StrictMock<MockTaskQueue>>();
    }

    void TearDown() override
    {

    }

    StrictMock<MockPluginManager> m_mockPluginManager;
    std::shared_ptr<StrictMock<MockTaskQueue>> m_mockTaskQueue;
private:
    std::unique_ptr<ManagementAgent::LoggerImpl::LoggingSetup> m_loggingSetup;
};

TEST_F(TaskDirectoryListenerTests, CheckListenerThrowsAwayUnknownFiles)
{
    std::string filePath = "/tmp/base/policy"; //"/opt/sophos-sspl/base/policy"
    std::string filename = "appId1-unknown.txt";
    std::string file1 = Common::FileSystem::join(filePath, filename);



    EXPECT_CALL(*m_mockTaskQueue, queueTask(_)).Times(0);

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener>
            listener1(new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(filePath, m_mockTaskQueue, m_mockPluginManager));

    listener1->fileMoved(filename);
}