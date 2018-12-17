/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockPluginManager.h"
#include "MockTaskQueue.h"

#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>
#include <Common/TaskQueueImpl/TaskProcessorImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/TestHelpers/MockFileSystem.h>

#include <ManagementAgent/McsRouterPluginCommunicationImpl/TaskDirectoryListener.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>

#include <tests/Common/Logging/TestConsoleLoggingSetup.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class TaskDirectoryListenerTests : public ::testing::Test
{
public:
    TaskDirectoryListenerTests() = default;

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
    TestLogging::TestConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TaskDirectoryListenerTests, CheckListenerThrowsAwayUnknownFiles) //NOLINT
{
    std::string filePath = "/tmp/base/policy"; //"/opt/sophos-sspl/base/policy"
    std::string filename = "appId1-unknown.txt";
    std::string file1 = Common::FileSystem::join(filePath, filename);



    EXPECT_CALL(*m_mockTaskQueue, queueTask(_)).Times(0);

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener>
            listener1(new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(filePath, m_mockTaskQueue, m_mockPluginManager));

    listener1->fileMoved(filename);
}