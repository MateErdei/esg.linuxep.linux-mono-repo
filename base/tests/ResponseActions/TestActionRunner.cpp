// Copyright 2023 Sophos Limited. All rights reserved.

#include "ResponseActions/ResponsePlugin/ActionRunner.h"
#include "ResponseActions/ResponsePlugin/TelemetryConsts.h"
#include "ResponseActions/RACommon/ResponseActionsCommon.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "ProcessImpl/ProcessImpl.h"

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockProcess.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ResponseActions::RACommon;
using namespace ResponsePlugin;
using namespace ResponsePlugin::Telemetry;
using namespace Common::Process;

class TestActionRunner  : public MemoryAppenderUsingTests
{
    public:
        TestActionRunner()
            : MemoryAppenderUsingTests("responseactions")
        {}
        void SetUp()
        {
        }

};

TEST_F(TestActionRunner, Success)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    ActionRunner actionRunner;

    actionRunner.runAction("action", "correlationid", "type", 34);
    EXPECT_TRUE(waitForLog("Trigger process at: /opt/sophos-spl/plugins/responseactions/bin/sophos_actions for action: correlationid with timeout: 34"));
    EXPECT_TRUE(waitForLog("output: some output"));
    EXPECT_TRUE(waitForLog("Action correlationid has succeeded"));
}

TEST_F(TestActionRunner, Timeout_ProcessExitsCleanly)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_,"sophos-spl-user","sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_,_)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, writeFile(_,_)).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_,_)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_))
                .WillOnce(Return(Common::Process::ProcessStatus::TIMEOUT))
                .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, sendSIGUSR1()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(2));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    ActionRunner actionRunner;

    actionRunner.runAction("action", "correlationid", "type", 100);
    EXPECT_TRUE(waitForLog("output: some output"));
    EXPECT_TRUE(waitForLog("Failed action correlationid with exit code 2"));
    EXPECT_TRUE(waitForLog("Action runner reached time out of 100 secs, correlation ID: correlationid"));
    EXPECT_TRUE(waitForLog("Response Actions plugin sending failed response to Central on behalf of Action Runner process"));
}

TEST_F(TestActionRunner, Timeout_ProcessExitsHanging)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_,"sophos-spl-user","sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_,_)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, writeFile(_,_)).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_,_)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_))
                .Times(2).WillRepeatedly(Return(Common::Process::ProcessStatus::TIMEOUT));
            EXPECT_CALL(*mockProcess, sendSIGUSR1()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(2));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    ActionRunner actionRunner;

    actionRunner.runAction("action", "correlationid", "type", 100);
    EXPECT_TRUE(waitForLog("output: some output"));
    EXPECT_TRUE(waitForLog("Failed action correlationid with exit code 2"));
    EXPECT_TRUE(waitForLog("Action Runner had to be killed after it carried on running unexpectedly"));
    EXPECT_TRUE(waitForLog("Action runner reached time out of 100 secs, correlation ID: correlationid"));
    EXPECT_TRUE(waitForLog("Response Actions plugin sending failed response to Central on behalf of Action Runner process"));
}

TEST_F(TestActionRunner, ProcessRunning_RespondsToSIGTERM)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

       Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(false));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    ActionRunner actionRunner;

    actionRunner.runAction("action", "correlationid", "type", 100);
    EXPECT_TRUE(waitForLog("output: some output"));
    EXPECT_TRUE(waitForLog("Action correlationid has succeeded"));
    EXPECT_TRUE(waitForLog("Action Runner was stopped after it carried on running unexpectedly"));
}

TEST_F(TestActionRunner, ProcessRunning_RequiresSIGKILL)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_,"sophos-spl-user","sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_,_)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, writeFile(_,_)).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_,_)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(9));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    ActionRunner actionRunner;

    actionRunner.runAction("action", "correlationid", "type", 100);
    EXPECT_TRUE(waitForLog("output: some output"));
    EXPECT_TRUE(waitForLog("Failed action correlationid with exit code 9"));
    EXPECT_TRUE(waitForLog("Action Runner had to be killed after it carried on running unexpectedly"));
    EXPECT_TRUE(waitForLog("Response Actions plugin sending failed response to Central on behalf of Action Runner process"));
}

class TestActionRunnerParameterized
    : public ::testing::TestWithParam<std::pair<std::string, std::string>>
{
protected:
    void SetUp() override
    {
        m_loggingSetup = Common::Logging::LOGOFFFORTEST();
    }
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};


INSTANTIATE_TEST_SUITE_P(
    TestActionRunner,
    TestActionRunnerParameterized,
    ::testing::Values(
        std::make_pair(RUN_COMMAND_REQUEST_TYPE, RUN_COMMAND_RESPONSE_TYPE),
        std::make_pair(UPLOAD_FILE_REQUEST_TYPE, UPLOAD_FILE_RESPONSE_TYPE),
        std::make_pair(UPLOAD_FOLDER_REQUEST_TYPE, UPLOAD_FOLDER_RESPONSE_TYPE),
        std::make_pair(DOWNLOAD_FILE_REQUEST_TYPE, DOWNLOAD_FILE_RESPONSE_TYPE)
            ));

TEST_P(TestActionRunnerParameterized, SendFailedResponseAssignsCorrectTypes)
{
    auto [ requestType, responseType ] = GetParam();

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_,"sophos-spl-user","sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_,_)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [&]()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(2));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);

        });
    ActionRunner actionRunner;

    std::stringstream expected;
    expected << R"({"result":3,"type":")" << responseType << R"("})";
    //We want to see the response type in the second argument of write file
    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, writeFile(_,expected.str())).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_,_)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    actionRunner.runAction("action", "correlationid", requestType, 1);
    while (actionRunner.getIsRunning()) {}
}


//Telemetry Tests

class TestTelemetryParameterized
    : public ::testing::TestWithParam<std::pair<std::string, std::vector<std::string>>>
{
protected:
    void SetUp() override
    {
        m_loggingSetup = Common::Logging::LOGOFFFORTEST();
    }
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

class TestSuccessTelemetry : public TestTelemetryParameterized {};

INSTANTIATE_TEST_SUITE_P(
    TestActionRunner,
    TestSuccessTelemetry,
    ::testing::Values(
        std::make_pair(RUN_COMMAND_REQUEST_TYPE, std::vector<std::string>{RUN_COMMAND_COUNT}),
        std::make_pair(UPLOAD_FILE_REQUEST_TYPE, std::vector<std::string>{UPLOAD_FILE_COUNT}),
        std::make_pair(UPLOAD_FOLDER_REQUEST_TYPE, std::vector<std::string>{UPLOAD_FOLDER_COUNT}),
        std::make_pair(DOWNLOAD_FILE_REQUEST_TYPE, std::vector<std::string>{DOWNLOAD_FILE_COUNT})
            ));

TEST_P(TestSuccessTelemetry, Success_Telemetry_Increments_Action)
{
    auto [ requestType, expectCountFieldName ] = GetParam();

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [&]()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);

        });
    ActionRunner actionRunner;

    actionRunner.runAction("action", "correlationid", requestType, 1);
    while (actionRunner.getIsRunning()) {}
    auto out = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    std::stringstream expected;
    expected << R"({")" << expectCountFieldName.front() << R"(":1})";
    EXPECT_EQ(out, expected.str());
}

class TestFailureTelemetry : public TestTelemetryParameterized {};

INSTANTIATE_TEST_SUITE_P(
    TestActionRunner,
    TestFailureTelemetry,
    ::testing::Values(
        std::make_pair(RUN_COMMAND_REQUEST_TYPE, std::vector<std::string>{RUN_COMMAND_COUNT, RUN_COMMAND_FAILED_COUNT}),
        std::make_pair(UPLOAD_FILE_REQUEST_TYPE, std::vector<std::string>{UPLOAD_FILE_COUNT, UPLOAD_FILE_FAILED_COUNT}),
        std::make_pair(UPLOAD_FOLDER_REQUEST_TYPE, std::vector<std::string>{UPLOAD_FOLDER_COUNT, UPLOAD_FOLDER_FAILED_COUNT}),
        std::make_pair(DOWNLOAD_FILE_REQUEST_TYPE, std::vector<std::string>{DOWNLOAD_FILE_COUNT, DOWNLOAD_FILE_FAILED_COUNT})
            ));

TEST_P(TestFailureTelemetry, Failed_Telemetry_Increments_Action)
{
    auto [ requestType, requestCountFieldName ] = GetParam();

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    ActionRunner actionRunner;

    actionRunner.runAction("action", "correlationid", requestType, 1);
    while (actionRunner.getIsRunning()) {}
    auto out = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    std::stringstream expected;
    expected << "{";
    for ( const auto& field : requestCountFieldName)
    {
        expected << R"(")" << field << R"(":1,)";
    }
    expected.seekp(-1, std::ios_base::end);
    expected << "}";

    EXPECT_EQ(out, expected.str());
}

class TestTimeoutTelemetry : public TestTelemetryParameterized {};

INSTANTIATE_TEST_SUITE_P(
    TestActionRunner,
    TestTimeoutTelemetry,
    ::testing::Values(
        std::make_pair(RUN_COMMAND_REQUEST_TYPE, std::vector<std::string>{RUN_COMMAND_COUNT, RUN_COMMAND_FAILED_COUNT, RUN_COMMAND_TIMEOUT_COUNT}),
        std::make_pair(UPLOAD_FILE_REQUEST_TYPE, std::vector<std::string>{UPLOAD_FILE_COUNT, UPLOAD_FILE_FAILED_COUNT, UPLOAD_FILE_TIMEOUT_COUNT}),
        std::make_pair(UPLOAD_FOLDER_REQUEST_TYPE, std::vector<std::string>{UPLOAD_FOLDER_COUNT, UPLOAD_FOLDER_FAILED_COUNT, UPLOAD_FOLDER_TIMEOUT_COUNT}),
        std::make_pair(DOWNLOAD_FILE_REQUEST_TYPE, std::vector<std::string>{DOWNLOAD_FILE_COUNT, DOWNLOAD_FILE_FAILED_COUNT, DOWNLOAD_FILE_TIMEOUT_COUNT})
            ));

TEST_P(TestTimeoutTelemetry, Timeout_Telemetry_Increments_Action)
{
    auto [ requestType, requestCountFieldName ] = GetParam();

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_,"sophos-spl-user","sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_,_)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, writeFile(_,_)).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_,_)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_))
                .WillOnce(Return(Common::Process::ProcessStatus::TIMEOUT))
                .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, sendSIGUSR1()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(2));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    ActionRunner actionRunner;

    actionRunner.runAction("action", "correlationid", requestType, 1);
    while (actionRunner.getIsRunning()) {}
    auto out = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    std::stringstream expected;
    expected << "{";
    for ( const auto& field : requestCountFieldName)
    {
        expected << R"(")" << field << R"(":1,)";
    }
    expected.seekp(-1, std::ios_base::end);
    expected << "}";

    EXPECT_EQ(out, expected.str());
}

TEST_P(TestTimeoutTelemetry, Hang_Telemetry_Increments_Action)
{
    auto [ requestType, requestCountFieldName ] = GetParam();

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_,"sophos-spl-user","sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_,_)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, writeFile(_,_)).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_,_)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_))
                .WillOnce(Return(Common::Process::ProcessStatus::TIMEOUT))
                .WillOnce(Return(Common::Process::ProcessStatus::TIMEOUT));
            EXPECT_CALL(*mockProcess, sendSIGUSR1()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill()).Times(1);
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(9));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    ActionRunner actionRunner;

    actionRunner.runAction("action", "correlationid", requestType, 1);
    while (actionRunner.getIsRunning()) {}
    auto out = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    std::stringstream expected;
    expected << "{";
    for ( const auto& field : requestCountFieldName)
    {
        expected << R"(")" << field << R"(":1,)";
    }
    expected.seekp(-1, std::ios_base::end);
    expected << "}";

    EXPECT_EQ(out, expected.str());
}

class TestExpiredTelemetry : public TestTelemetryParameterized {};

INSTANTIATE_TEST_SUITE_P(
    TestActionRunner,
    TestExpiredTelemetry,
    ::testing::Values(
        std::make_pair(RUN_COMMAND_REQUEST_TYPE, std::vector<std::string>{RUN_COMMAND_COUNT, RUN_COMMAND_EXPIRED_COUNT, RUN_COMMAND_FAILED_COUNT}),
        std::make_pair(UPLOAD_FILE_REQUEST_TYPE, std::vector<std::string>{UPLOAD_FILE_COUNT, UPLOAD_FILE_EXPIRED_COUNT, UPLOAD_FILE_FAILED_COUNT}),
        std::make_pair(UPLOAD_FOLDER_REQUEST_TYPE, std::vector<std::string>{UPLOAD_FOLDER_COUNT, UPLOAD_FOLDER_EXPIRED_COUNT, UPLOAD_FOLDER_FAILED_COUNT}),
        std::make_pair(DOWNLOAD_FILE_REQUEST_TYPE, std::vector<std::string>{DOWNLOAD_FILE_COUNT, DOWNLOAD_FILE_EXPIRED_COUNT, DOWNLOAD_FILE_FAILED_COUNT})
            ));

TEST_P(TestExpiredTelemetry, Expired_Telemetry_Increments_Action)
{
    auto [ requestType, requestCountFieldName ] = GetParam();

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(4));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    ActionRunner actionRunner;

    actionRunner.runAction("action", "correlationid", requestType, 1);
    while (actionRunner.getIsRunning()) {}
    auto out = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    std::stringstream expected;
    expected << "{";
    for ( const auto& field : requestCountFieldName)
    {
        expected << R"(")" << field << R"(":1,)";
    }
    expected.seekp(-1, std::ios_base::end);
    expected << "}";

    EXPECT_EQ(out, expected.str());
}