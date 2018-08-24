/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>

#include "MockPluginManager.h"

#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <ManagementAgent/LoggerImpl/LoggingSetup.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>

class PolicyTaskTests : public ::testing::Test
{
public:
    PolicyTaskTests()
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

TEST_F(PolicyTaskTests, PolicyTaskAssignsPolicyWhenRun) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("SAV", "Hello")).WillOnce(Return(1));

    StrictMock<MockFileSystem> *filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("Hello"));
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));


    ManagementAgent::McsRouterPluginCommunicationImpl::PolicyTask task(m_mockPluginManager,"/tmp/policy/SAV-11_policy.xml");
    task.run();

    Common::FileSystem::restoreFileSystem();
}

TEST_F(PolicyTaskTests, PolicyTaskHandlesNameWithoutHyphen) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy(_,_)).Times(0);

    ManagementAgent::McsRouterPluginCommunicationImpl::PolicyTask task(m_mockPluginManager,"/tmp/policy/PolicyTaskHandlesNameWithoutHyphen");
    task.run();
}