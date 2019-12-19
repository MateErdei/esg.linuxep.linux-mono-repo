/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockPluginManager.h"

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/ActionTask.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>

using ::testing::_;

class ActionTaskTests : public ::testing::Test
{
public:
    ActionTaskTests() = default;

    void SetUp() override {}

    void TearDown() override
    {
        Tests::restoreFileSystem();
    }

    StrictMock<MockPluginManager> m_mockPluginManager;

private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(ActionTaskTests, ActionTaskQueuesActionWhenRun) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction("SAV", "Hello", "")).WillOnce(Return(1));

    auto filesystemMock = new NiceMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("Hello"));
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
        m_mockPluginManager, "/tmp/action/SAV_action_11.xml");
    task.run();


}

TEST_F(ActionTaskTests, ActionTaskDeletesActionFileOnceQueued) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction("SAV", "Hello", "")).WillOnce(Return(1));

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("Hello"));
    EXPECT_CALL(*filesystemMock, removeFile(_)).Times(1);
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
        m_mockPluginManager, "/tmp/action/SAV_action_11.xml");
    task.run();

}

TEST_F(ActionTaskTests, ActionTaskHandlesNameWithoutHyphen) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction(_, _, _)).Times(0);

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
        m_mockPluginManager, "/tmp/action/ActionTaskHandlesNameWithoutHyphen");
    task.run();
}

TEST_F(ActionTaskTests, LiveQueryFilesWillBeForwardedToPlugin) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction("LiveQuery", "FileContent", "correlation-id")).WillOnce(Return(1));

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("FileContent"));
    EXPECT_CALL(*filesystemMock, removeFile(_)).Times(1);
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, "/tmp/action/LiveQuery_correlation-id_2013-05-02T09:50:08Z_request.json");
    task.run();
}

TEST_F(ActionTaskTests, LiveQueryFilesWithoutAppIdWillBeDiscardedAndErrorLogged) // NOLINT
{

    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup loggingSetup;

    EXPECT_CALL(m_mockPluginManager, queueAction(_,_,_)).Times(0);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, "/tmp/action/correlation-id_2013-05-02T09:50:08Z_request.json");
    task.run();
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("invalid"));
}

TEST_F(ActionTaskTests, LiveQueryFilesWithoutCorrelationIdWillBeDiscardedAndErrorLogged) // NOLINT
{

    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup loggingSetup;

    EXPECT_CALL(m_mockPluginManager, queueAction(_,_,_)).Times(0);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, "/tmp/action/LiveQuery_2013-05-02T09:50:08Z_request.json");
    task.run();
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("invalid"));
}

TEST_F(ActionTaskTests, LiveQueryFilesWithInvalidConventionNameWillBeDiscarded) // NOLINT
{

    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup loggingSetup;

    EXPECT_CALL(m_mockPluginManager, queueAction(_,_,_)).Times(0);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, "/tmp/action/ThisIsNotARequestAtAll.json");
    task.run();
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("invalid"));
}
