/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockPluginManager.h"

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>

class PolicyTaskTests : public ::testing::Test
{
public:
    PolicyTaskTests() = default;

    void SetUp() override {}

    void TearDown() override {}

    StrictMock<MockPluginManager> m_mockPluginManager;

private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(PolicyTaskTests, PolicyTaskAssignsPolicyWhenRun) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("SAV", "Hello")).WillOnce(Return(1));

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("Hello"));
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    ManagementAgent::McsRouterPluginCommunicationImpl::PolicyTask task(
        m_mockPluginManager, "/tmp/policy/SAV-11_policy.xml");
    task.run();

    Tests::restoreFileSystem();
}

TEST_F(PolicyTaskTests, PolicyTaskHandlesNameWithoutHyphen) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy(_, _)).Times(0);

    ManagementAgent::McsRouterPluginCommunicationImpl::PolicyTask task(
        m_mockPluginManager, "/tmp/policy/PolicyTaskHandlesNameWithoutHyphen");
    task.run();
}