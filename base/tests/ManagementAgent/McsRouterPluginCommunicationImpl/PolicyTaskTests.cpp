// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/ManagementAgent/MockPluginManager/MockPluginManager.h"

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

TEST_F(PolicyTaskTests, PolicyTaskAssignsPolicyWhenRun)
{
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("SAV", "SAV-11_policy.xml", "test-plugin")).WillOnce(Return(1));

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    ManagementAgent::McsRouterPluginCommunicationImpl::PolicyTask task(
        m_mockPluginManager, "SAV-11_policy.xml", "test-plugin");
    task.run();

}

TEST_F(PolicyTaskTests, PolicyTaskHandlesNameWithoutHyphen)
{
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy(_, _, "test-plugin")).Times(0);

    ManagementAgent::McsRouterPluginCommunicationImpl::PolicyTask task(
        m_mockPluginManager, "/tmp/policy/PolicyTaskHandlesNameWithoutHyphen", "test-plugin");
    task.run();
}