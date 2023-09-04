// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "ManagementAgent/McsRouterPluginCommunicationImpl/ActionTask.h"
#include "tests/Common/Helpers/FakeTimeUtils.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/ManagementAgent/MockPluginManager/MockPluginManager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

std::time_t t_20190501T13h{ 1556712000 };
std::time_t t_20200610T12h{ 1591790400 };

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
    EXPECT_CALL(m_mockPluginManager, queueAction("SAV", "SAV_action_11.xml", "")).WillOnce(Return(1));

    auto filesystemMock = new NiceMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
        m_mockPluginManager, "SAV_action_11.xml");
    task.run();


}

TEST_F(ActionTaskTests, ActionTaskHandlesNameWithoutHyphen) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction(_, _, _)).Times(0);

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
        m_mockPluginManager, "ActionTaskHandlesNameWithoutHyphen");
    task.run();
}

TEST_F(ActionTaskTests, ActionTaskHandlesNameWithSingleUnderscore) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction(_, _, _)).Times(0);

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, "_");
    task.run();
}

TEST_F(ActionTaskTests, LiveQueryFilesWillBeForwardedToPlugin) // NOLINT
{
    EXPECT_CALL(m_mockPluginManager, queueAction("LiveQuery", "LiveQuery_correlation-id_request_2013-05-02T09:50:08Z_1591790400.json", "correlation-id")).WillOnce(Return(1));

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    bool stop{ false };
    // make managementagent think it is 20190501T13h so that the action ttl we give is in the past
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20190501T13h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));
    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, "LiveQuery_correlation-id_request_2013-05-02T09:50:08Z_1591790400.json");
    task.run();
}

TEST_F(ActionTaskTests, LiveQueryFilesWithoutAppIdWillBeDiscardedAndErrorLogged) // NOLINT
{

    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup loggingSetup;

    EXPECT_CALL(m_mockPluginManager, queueAction(_,_,_)).Times(0);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    bool stop{ false };
    // make managementagent think it is 20190501T13h so that the action ttl we give is in the past
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20190501T13h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));
    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, "correlation-id_request_2013-05-02T09:50:08Z_1591790400.json");
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

    bool stop{ false };
    // make managementagent think it is 20190501T13h so that the action ttl we give is in the past
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20190501T13h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));
    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, "LiveQuery_request_2013-05-02T09:50:08Z_1591790400.json");
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
            m_mockPluginManager, "ThisIsNotARequestAtAll.json");
    task.run();
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("invalid"));
}

TEST_F(ActionTaskTests, LiveQueryFileWithExpiredTTLIsDiscarded)
{
    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup loggingSetup;

    EXPECT_CALL(m_mockPluginManager, queueAction(_,_,_)).Times(0);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    bool stop{ false };
    // make managementagent think it is 20200610T12h so that the action ttl we give is in the past
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20200610T12h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, "LiveQuery_correlation-id_request_2020-06-09T09:15:23Z_1556712000.json");
    task.run();
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("Action has expired"));
}

TEST_F(ActionTaskTests, LiveQueryFileWithTimeToLiveEqualToCurrentTimeIsProcessed)
{
    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup loggingSetup;
    std::string file_path = "LiveTerminal_correlation-id_action_2020-06-09T09:15:23Z_1591790400.xml";
    EXPECT_CALL(m_mockPluginManager, queueAction("LiveTerminal",file_path,"correlation-id")).Times(1);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    bool stop{ false };
    // make managementagent think it is 20190501T13h so that the action ttl we give is in the past
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20200610T12h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, file_path);
    task.run();
    std::string logMessage = testing::internal::GetCapturedStderr();
}


TEST_F(ActionTaskTests, LiveResponseFileWithExpiredTTLIsDiscarded)
{
    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup loggingSetup;

    EXPECT_CALL(m_mockPluginManager, queueAction(_,_,_)).Times(0);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    bool stop{ false };
    // make managementagent think it is 20200610T12h so that the action ttl we give is in the past
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20200610T12h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, "LiveTerminal_correlation-id_action_2020-06-09T09:15:23Z_1556712000.json");
    task.run();
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("Action has expired"));
}


TEST_F(ActionTaskTests, LiveResponseFileWithTimeToLiveInFutureIsProcessed)
{
    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup loggingSetup;
    std::string file_path = "LiveTerminal_correlation-id_action_2020-06-09T09:15:23Z_1591790400.json";
    EXPECT_CALL(m_mockPluginManager, queueAction("LiveTerminal",file_path, "correlation-id")).Times(1);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    bool stop{ false };
    // make managementagent think it is 20190501T13h so that the action ttl we give is in the past
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20190501T13h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, file_path);
    task.run();
    std::string logMessage = testing::internal::GetCapturedStderr();

}

TEST_F(ActionTaskTests, LiveResponseFileWithTimeToLiveEqualToCurrentTimeIsProcessed)
{
    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup loggingSetup;
    std::string file_path = "LiveTerminal_correlation-id_action_2020-06-09T09:15:23Z_1591790400.xml";
    EXPECT_CALL(m_mockPluginManager, queueAction("LiveTerminal",file_path,"correlation-id")).Times(1);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    bool stop{ false };
    // make managementagent think it is 20190501T13h so that the action ttl we give is in the past
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20200610T12h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
            m_mockPluginManager, file_path);
    task.run();
    std::string logMessage = testing::internal::GetCapturedStderr();
}

TEST_F(ActionTaskTests, TooLongAndTooShortLiveResponseFilenamesNotProcessedAsGenericActions)
{
    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup loggingSetup;

    EXPECT_CALL(m_mockPluginManager, queueAction("LiveTerminal",_,_)).Times(0);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readFile(_)).Times(0);
    std::string file_path1 = "LiveTerminal_correlation-id_action_123456.xml";
    std::string file_path2 = "LiveTerminal_extra-argument_correlation-id_action_FakeTime_123456.xml";
    EXPECT_CALL(*filesystemMock, removeFile(file_path1, _)).Times(0);
    EXPECT_CALL(*filesystemMock, removeFile(file_path2, _)).Times(0);
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    bool stop{ false };
    // make managementagent think it is 20190501T13h so that the action ttl we give is in the past
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20200610T12h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task1(
            m_mockPluginManager, file_path1);
    task1.run();
    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task2(
            m_mockPluginManager, file_path2);
    task2.run();
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("invalid"));
}

TEST_F(ActionTaskTests, TooLongAndTooShortLiveQueryFilenamesNotProcessedAsGenericActions)
{
    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup loggingSetup;

    EXPECT_CALL(m_mockPluginManager, queueAction("LiveQuery",_,_)).Times(0);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readFile(_)).Times(0);
    std::string file_path1 = "LiveQuery_correlation-id_request_123456.xml";
    std::string file_path2 = "LiveQuery_extra-argument_correlation-id_request_FakeTime_123456.xml";
    EXPECT_CALL(*filesystemMock, removeFile(file_path1, _)).Times(0);
    EXPECT_CALL(*filesystemMock, removeFile(file_path2, _)).Times(0);
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    bool stop{ false };
    // make managementagent think it is 20190501T13h so that the action ttl we give is in the past
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {t_20200610T12h}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));

    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task1(
            m_mockPluginManager, file_path1);
    task1.run();
    ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task2(
            m_mockPluginManager, file_path2);
    task2.run();
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("invalid"));
}

TEST_F(ActionTaskTests, TestIsAliveMethod)
{
    bool stop{ false };
    // make method think the current time is 1591790400 so that the action ttl we give is in the past
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime{ {1591790400}, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));

    EXPECT_EQ(ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask::isAlive("0"), false);
    EXPECT_EQ(ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask::isAlive("1591790399"), false);
    EXPECT_EQ(ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask::isAlive("1591790400"), true);
    EXPECT_EQ(ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask::isAlive("1591790401"), true);
}

TEST_F(ActionTaskTests, testIsAliveWithBadArgumentThrows)
{
     EXPECT_THROW(ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask::isAlive("IAmNotANumber"),
                  ManagementAgent::McsRouterPluginCommunicationImpl::FailedToConvertTtlException);
}

TEST_F(ActionTaskTests, TestGarbageTTLIsHandledWithLogging) // NOLINT
{

    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup loggingSetup;

    EXPECT_CALL(m_mockPluginManager, queueAction(_,_,_)).Times(0);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    try
    {
        ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask task(
                m_mockPluginManager, "LiveTerminal_action_2020-06-09T09:15:23Z_IAmNotANumber.json");
    }
    catch(ManagementAgent::McsRouterPluginCommunicationImpl::FailedToConvertTtlException& exception)
    {
        EXPECT_STREQ(exception.what(), "Failed to convert time to live 'IAmNotANumber' into time_t");
    }
    catch(...)
    {
        FAIL();
    }
}