/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockPluginManager.h"

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/ActionTask.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Logging/TestConsoleLoggingSetup.h>

using ::testing::_;

class ActionTaskTests : public ::testing::Test
{
public:
    ActionTaskTests() = default;

    void SetUp() override {}

    void TearDown() override {}

    StrictMock<MockPluginManager> m_mockPluginManager;

private:
    TestLogging::TestConsoleLoggingSetup m_loggingSetup;
};

TEST_F(ActionTaskTests, ActionTaskQueuesActionWhenRun) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction("SAV", "Hello")).WillOnce(Return(1));

    auto filesystemMock = new NiceMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("Hello"));
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
        m_mockPluginManager, "/tmp/action/SAV_action_11.xml");
    task.run();

    Tests::restoreFileSystem();
}

TEST_F(ActionTaskTests, ActionTaskDeletesActionFileOnceQueued) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction("SAV", "Hello")).WillOnce(Return(1));

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("Hello"));
    EXPECT_CALL(*filesystemMock, removeFile(_)).Times(1);
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
        m_mockPluginManager, "/tmp/action/SAV_action_11.xml");
    task.run();

    Tests::restoreFileSystem();
}

TEST_F(ActionTaskTests, ActionTaskHandlesNameWithoutHyphen) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction(_, _)).Times(0);

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
        m_mockPluginManager, "/tmp/action/ActionTaskHandlesNameWithoutHyphen");
    task.run();
}