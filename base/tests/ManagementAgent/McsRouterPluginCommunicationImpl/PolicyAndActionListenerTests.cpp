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
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyAndActionListener.h>

#include "TempDir.h"
#include "MockPluginManager.h"
#include "MockTaskQueue.h"

class PolicyAndActionListenerTests : public ::testing::Test
{
public:

    void SetUp() override
    {
        m_mockTaskQueue = std::make_shared<StrictMock<MockTaskQueue>>();
        m_tempDir = Tests::TempDir::makeTempDir();
    }

    void TearDown() override
    {
        m_tempDir.reset(nullptr);
    }

    StrictMock<MockPluginManager> m_mockPluginManager;
    std::shared_ptr<StrictMock<MockTaskQueue>> m_mockTaskQueue;
    std::unique_ptr<Tests::TempDir> m_tempDir;

};

TEST_F(PolicyAndActionListenerTests, Construction) // NOLINT
{
    m_tempDir->makeDirs("policy");
    m_tempDir->makeDirs("action");

    auto directoryWatcher = std::make_shared<Common::DirectoryWatcherImpl::DirectoryWatcher>();

    ManagementAgent::McsRouterPluginCommunicationImpl::PolicyAndActionListener policyAndActionListener(
            m_tempDir->dirPath(),
            directoryWatcher,
            m_mockTaskQueue,
            m_mockPluginManager);
}