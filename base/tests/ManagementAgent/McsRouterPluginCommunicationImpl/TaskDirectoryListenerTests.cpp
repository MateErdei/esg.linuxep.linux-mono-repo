/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>
#include <Common/TaskQueue/ITaskProcessor.h>
#include <Common/TaskQueueImpl/TaskProcessorImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/TaskDirectoryListener.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <modules/Common/FileSystemImpl/FileSystemImpl.h>
#include <modules/ManagementAgent/McsRouterPluginCommunicationImpl/PolicyAndActionListener.h>

#include "TempDir.h"
#include "MockPluginManager.h"
#include "MockTaskQueue.h"

class TaskDirectoryListenerTests : public ::testing::Test
{
public:

    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
};

TEST_F(TaskDirectoryListenerTests, CheckListenerThrowsAwayUnknownFiles)
{

    std::string filePath = "/tmp/base/policy"; //"/opt/sophos-sspl/base/policy"
    std::string filename = "appId1-unknown.txt";
    std::string file1 = Common::FileSystem::fileSystem()->join(filePath, filename);

    MockPluginManager mockPluginManager;
    std::shared_ptr<MockTaskQueue> mockTaskQueue(new MockTaskQueue());

    EXPECT_CALL(*mockTaskQueue, queueTask(_)).Times(0);

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener>
            listener1(new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(filePath,mockTaskQueue,mockPluginManager));

    listener1->fileMoved(filename);
}