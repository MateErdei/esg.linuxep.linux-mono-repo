/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <modules/ManagementAgent/LoggerImpl/LoggingSetup.h>

#include "ManagementAgent/McsRouterPluginCommunicationImpl/ActionTask.h"
#include "tests/Common/FileSystemImpl/MockFileSystem.h"
#include "modules/Common/FileSystemImpl/FileSystemImpl.h"
#include "MockPluginManager.h"

using ::testing::_;

class ActionTaskTests : public ::testing::Test
{
public:
    ActionTaskTests()
    : m_loggingSetup(std::unique_ptr<ManagementAgent::LoggerImpl::LoggingSetup>(new ManagementAgent::LoggerImpl::LoggingSetup(1)))
    {

    }
    void SetUp() override
    {

    }

    void TearDown() override
    {

    }

    StrictMock<MockPluginManager> m_mockPluginManager;
private:
    std::unique_ptr<ManagementAgent::LoggerImpl::LoggingSetup> m_loggingSetup;
};

TEST_F(ActionTaskTests, ActionTaskQueuesActionWhenRun) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction("SAV", "Hello")).WillOnce(Return(1));

    NiceMock<MockFileSystem> *filesystemMock = new NiceMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("Hello"));
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));


    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(m_mockPluginManager,
                                                                       "/tmp/action/SAV_action_11.xml"
    );
    task.run();

    Common::FileSystem::restoreFileSystem();
}

TEST_F(ActionTaskTests, ActionTaskDeletesActionFileOnceQueued) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction("SAV", "Hello")).WillOnce(Return(1));

    StrictMock<MockFileSystem> *filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("Hello"));
    EXPECT_CALL(*filesystemMock, removeFile(_)).Times(1);
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(m_mockPluginManager,
                                                                       "/tmp/action/SAV_action_11.xml"
    );
    task.run();

    Common::FileSystem::restoreFileSystem();
}

TEST_F(ActionTaskTests, ActionTaskHandlesNameWithoutHyphen) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction(_,_)).Times(0);

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(m_mockPluginManager,"/tmp/action/ActionTaskHandlesNameWithoutHyphen");
    task.run();
}