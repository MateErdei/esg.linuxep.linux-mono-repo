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

class PolicyAndActionListenerTests : public ::testing::Test
{
public:

    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
};

TEST_F(PolicyAndActionListenerTests, Construction) // NOLINT
{
Tests::TempDir tempDir;

tempDir.makeDirs("policy");
tempDir.makeDirs("action");

MockPluginManager mockPluginManager;
auto directoryWatcher = std::make_shared<Common::DirectoryWatcherImpl::DirectoryWatcher>();
auto mockTaskQueue = std::make_shared<MockTaskQueue>();

ManagementAgent::McsRouterPluginCommunicationImpl::PolicyAndActionListener policyAndActionListener(
        tempDir.dirPath(),
        directoryWatcher,
        mockTaskQueue,
        mockPluginManager
);
}