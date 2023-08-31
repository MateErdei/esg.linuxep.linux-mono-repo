// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/ProcessImpl/ProcessImpl.h"
#include "ResponseActions/RACommon/ResponseActionsCommon.h"
#include "ResponseActions/ResponsePlugin/ActionRunner.h"
#include "ResponseActions/ResponsePlugin/TelemetryConsts.h"
#include "tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h"
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

class TestActionRunner : public MemoryAppenderUsingTests
{
public:
    TestActionRunner() : MemoryAppenderUsingTests("responseactions") {}
    void SetUp() {}
};

TEST_F(TestActionRunner, Success)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);

    actionRunner.runAction("action", "correlationid", RUN_COMMAND_REQUEST_TYPE, 34);
    EXPECT_TRUE(waitForLog("Trigger process at: /opt/sophos-spl/plugins/responseactions/bin/sophos_actions for action: "
                           "correlationid with timeout: 34"));
    EXPECT_TRUE(waitForLog("output: some output"));
    EXPECT_TRUE(waitForLog("Action correlationid has succeeded"));
    EXPECT_EQ(queueTask->pop().m_taskType,
              ResponsePlugin::Task::TaskType::CHECK_QUEUE);
}

TEST_F(TestActionRunner, Timeout_ProcessExitsCleanly)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_, "sophos-spl-user", "sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_, _)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, writeFile(_, _)).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_, _)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _))
                .WillOnce(Return(Common::Process::ProcessStatus::TIMEOUT))
                .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, sendSIGUSR1()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(2));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);

    actionRunner.runAction("action", "correlationid", UPLOAD_FOLDER_REQUEST_TYPE, 100);
    EXPECT_TRUE(waitForLog("output: some output"));
    EXPECT_TRUE(waitForLog("Failed action correlationid with exit code 2"));
    EXPECT_TRUE(waitForLog("Action runner reached time out of 100 secs, correlation ID: correlationid"));
    EXPECT_TRUE(
        waitForLog("Response Actions plugin sending failed response to Central on behalf of Action Runner process"));
    EXPECT_EQ(queueTask->pop().m_taskType,
              ResponsePlugin::Task::TaskType::CHECK_QUEUE);
}

TEST_F(TestActionRunner, Timeout_ProcessExitsHanging)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_, "sophos-spl-user", "sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_, _)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, writeFile(_, _)).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_, _)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _))
                .Times(2)
                .WillRepeatedly(Return(Common::Process::ProcessStatus::TIMEOUT));
            EXPECT_CALL(*mockProcess, sendSIGUSR1()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(2));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);

    actionRunner.runAction("action", "correlationid", UPLOAD_FILE_REQUEST_TYPE, 100);
    EXPECT_TRUE(waitForLog("output: some output"));
    EXPECT_TRUE(waitForLog("Failed action correlationid with exit code 2"));
    EXPECT_TRUE(waitForLog("Action Runner had to be killed after it carried on running unexpectedly"));
    EXPECT_TRUE(waitForLog("Action runner reached time out of 100 secs, correlation ID: correlationid"));
    EXPECT_TRUE(
        waitForLog("Response Actions plugin sending failed response to Central on behalf of Action Runner process"));
    EXPECT_EQ(queueTask->pop().m_taskType,
              ResponsePlugin::Task::TaskType::CHECK_QUEUE);
}

TEST_F(TestActionRunner, ProcessRunning_RespondsToSIGTERM)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(false));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);

    actionRunner.runAction("action", "correlationid", DOWNLOAD_FILE_REQUEST_TYPE, 100);
    EXPECT_TRUE(waitForLog("output: some output"));
    EXPECT_TRUE(waitForLog("Action correlationid has succeeded"));
    EXPECT_TRUE(waitForLog("Action Runner was stopped after it carried on running unexpectedly"));
}

TEST_F(TestActionRunner, ProcessRunning_RequiresSIGKILL)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_, "sophos-spl-user", "sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_, _)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, writeFile(_, _)).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_, _)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(9));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);

    actionRunner.runAction("action", "correlationid", RUN_COMMAND_REQUEST_TYPE, 100);
    EXPECT_TRUE(waitForLog("output: some output"));
    EXPECT_TRUE(waitForLog("Failed action correlationid with exit code 9"));
    EXPECT_TRUE(waitForLog("Action Runner had to be killed after it carried on running unexpectedly"));
    EXPECT_TRUE(
        waitForLog("Response Actions plugin sending failed response to Central on behalf of Action Runner process"));
    EXPECT_EQ(queueTask->pop().m_taskType,
              ResponsePlugin::Task::TaskType::CHECK_QUEUE);
}

TEST_F(TestActionRunner, runActionHandlesMissingBinaryWithInternalErrorResultCode)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_, "sophos-spl-user", "sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_, _)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    // result 3 is INTERNAL_ERROR
    std::string expectedContent = R"({"result":3,"type":"sophos.mgt.response.RunCommands"})";
    auto filesystemMock = std::make_unique<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, writeFile(_, expectedContent)).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_, _)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(filesystemMock) };

    auto mockAppManager = std::make_unique<MockedApplicationPathManager>();
    MockedApplicationPathManager& mock(*mockAppManager);
    ON_CALL(mock, getResponseActionRunnerPath()).WillByDefault(Return("./this/is/not/a/binary"));
    Common::ApplicationConfiguration::replaceApplicationPathManager(std::move(mockAppManager));

    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);
    actionRunner.runAction("action", "correlationid", RUN_COMMAND_REQUEST_TYPE, 10);
    EXPECT_TRUE(
        waitForLog("Response Actions plugin sending failed response to Central on behalf of Action Runner process"));
    EXPECT_EQ(queueTask->pop().m_taskType,
              ResponsePlugin::Task::TaskType::CHECK_QUEUE);
}

TEST_F(TestActionRunner, runActionHandlesNonExecutableBinaryWithInternalErrorResultCode)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_, "sophos-spl-user", "sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_, _)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    // result 3 is INTERNAL_ERROR
    std::string expectedContent = R"({"result":3,"type":"sophos.mgt.response.RunCommands"})";
    auto filesystemMock = std::make_unique<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, writeFile(_, expectedContent)).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_, _)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(filesystemMock) };

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = std::make_unique<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));

            // Permission denied what we get when we try and execute something that is not executable.
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(13));
            return mockProcess;
        });

    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);
    actionRunner.runAction("action", "correlationid", RUN_COMMAND_REQUEST_TYPE, 10);
    EXPECT_TRUE(waitForLog("Failed action correlationid with exit code 13"));
    EXPECT_TRUE(
        waitForLog("Response Actions plugin sending failed response to Central on behalf of Action Runner process"));
    EXPECT_EQ(queueTask->pop().m_taskType,
              ResponsePlugin::Task::TaskType::CHECK_QUEUE);
}

class TestActionRunnerParameterized : public ::testing::TestWithParam<std::pair<std::string, std::string>>
{
protected:
    void SetUp() override { m_loggingSetup = Common::Logging::LOGOFFFORTEST(); }
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

INSTANTIATE_TEST_SUITE_P(
    TestActionRunner,
    TestActionRunnerParameterized,
    ::testing::Values(
        std::make_pair(RUN_COMMAND_REQUEST_TYPE, RUN_COMMAND_RESPONSE_TYPE),
        std::make_pair(UPLOAD_FILE_REQUEST_TYPE, UPLOAD_FILE_RESPONSE_TYPE),
        std::make_pair(UPLOAD_FOLDER_REQUEST_TYPE, UPLOAD_FOLDER_RESPONSE_TYPE),
        std::make_pair(DOWNLOAD_FILE_REQUEST_TYPE, DOWNLOAD_FILE_RESPONSE_TYPE)));

TEST_P(TestActionRunnerParameterized, SendFailedResponseAssignsCorrectTypes)
{
    auto [requestType, responseType] = GetParam();

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_, "sophos-spl-user", "sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_, _)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [&]()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(2));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });
    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);

    std::stringstream expected;
    expected << R"({"result":3,"type":")" << responseType << R"("})";
    // We want to see the response type in the second argument of write file
    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, writeFile(_, expected.str())).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_, _)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    actionRunner.runAction("action", "correlationid", requestType, 1);
    while (actionRunner.getIsRunning())
    {
    }
}

TEST_F(TestActionRunner, SendFailedResponseThrowsWithInvalidType)
{
    ResponseResult res{};

    try
    {
        ResponsePlugin::sendFailedResponse(res, "notatype", "correlationId");
        FAIL() << "Invalid type ignored";
    }
    catch (const std::logic_error& e)
    {
        EXPECT_STREQ(e.what(), "Unknown action type provided to sendFailedResponse: notatype");
    }
}

TEST_F(TestActionRunner, TestToUtf8)
{
    std::string threatPath = "abc  \1 \2 \3 \4 \5 \6 \\ efg \a \b \t \n \v \f \r hik";
    std::string utf8Path = *(ResponseActions::RACommon::toUtf8(threatPath));
    EXPECT_EQ(utf8Path, threatPath);
}

TEST_F(TestActionRunner, TestToUtf8WeirdCharacters)
{
    std::string threatPath = "ありったけの夢をかき集め \1 \2 \3 \4 \5 \6 \\ Ἄνδρα μοι ἔννεπε \a \b \t \n \v \f \r Ä Ö Ü ß";
    std::string utf8Path = *(ResponseActions::RACommon::toUtf8(threatPath));
    EXPECT_EQ(utf8Path, threatPath);
}

TEST_F(TestActionRunner, TestToUtf8FromEucJP)
{
    // echo -n "ありったけの夢をかき集め" | iconv -f utf-8 -t euc-jp | hexdump -C
    std::vector<unsigned char> threatPathBytes { 0xa4, 0xa2, 0xa4, 0xea, 0xa4, 0xc3, 0xa4, 0xbf, 0xa4, 0xb1, 0xa4, 0xce,
                                                0xcc, 0xb4, 0xa4, 0xf2, 0xa4, 0xab, 0xa4, 0xad, 0xbd, 0xb8, 0xa4, 0xe1 };
    std::string threatPath(threatPathBytes.begin(), threatPathBytes.end());

    std::string threatPathUtf8 = "ありったけの夢をかき集め";
    std::string utf8Path = *(ResponseActions::RACommon::toUtf8(threatPath, false));
    EXPECT_EQ(utf8Path, threatPathUtf8);

    threatPathUtf8 += " (EUC-JP)";
    utf8Path = *(ResponseActions::RACommon::toUtf8(threatPath, true));
    EXPECT_EQ(utf8Path, threatPathUtf8);

    utf8Path = *(ResponseActions::RACommon::toUtf8(threatPath));
    EXPECT_EQ(utf8Path, threatPathUtf8);
}

TEST_F(TestActionRunner, TestToUtf8FromSJIS)
{
    // echo -n "ありったけの夢をかき集め" | iconv -f utf-8 -t sjis | hexdump -C
    std::vector<unsigned char> threatPathBytes { 0x82, 0xa0, 0x82, 0xe8, 0x82, 0xc1, 0x82, 0xbd, 0x82, 0xaf, 0x82, 0xcc,
                                                0x96, 0xb2, 0x82, 0xf0, 0x82, 0xa9, 0x82, 0xab, 0x8f, 0x57, 0x82, 0xdf };
    std::string threatPath(threatPathBytes.begin(), threatPathBytes.end());

    std::string threatPathUtf8 = "ありったけの夢をかき集め";
    std::string utf8Path = *(ResponseActions::RACommon::toUtf8(threatPath, false));
    EXPECT_EQ(utf8Path, threatPathUtf8);

    threatPathUtf8 += " (Shift-JIS)";
    utf8Path = *(ResponseActions::RACommon::toUtf8(threatPath, true));
    EXPECT_EQ(utf8Path, threatPathUtf8);

    utf8Path = *(ResponseActions::RACommon::toUtf8(threatPath));
    EXPECT_EQ(utf8Path, threatPathUtf8);
}

TEST_F(TestActionRunner, TestToUtf8FromLatin1)
{
    // echo -n "Ä ö ü ß" | iconv -f utf-8 -t latin1 | hexdump -C
    std::vector<unsigned char> threatPathBytes{ 0xc4, 0x20, 0xf6, 0x20, 0xfc, 0x20, 0xdf };
    std::string threatPath(threatPathBytes.begin(), threatPathBytes.end());

    std::string threatPathUtf8 = "Ä ö ü ß";
    std::string utf8Path = *(ResponseActions::RACommon::toUtf8(threatPath, false));
    EXPECT_EQ(utf8Path, threatPathUtf8);

    threatPathUtf8 += " (Latin1)";
    utf8Path = *(ResponseActions::RACommon::toUtf8(threatPath, true));
    EXPECT_EQ(utf8Path, threatPathUtf8);

    utf8Path = *(ResponseActions::RACommon::toUtf8(threatPath));
    EXPECT_EQ(utf8Path, threatPathUtf8);
}

// Telemetry Tests

class TestTelemetryParameterized : public ::testing::TestWithParam<std::pair<std::string, std::vector<std::string>>>
{
protected:
    void SetUp() override
    {
        // Clear telemetry before a test
        std::ignore = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
        m_loggingSetup = Common::Logging::LOGOFFFORTEST();
    }
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

class TestSuccessTelemetry : public TestTelemetryParameterized
{
};

INSTANTIATE_TEST_SUITE_P(
    TestActionRunner,
    TestSuccessTelemetry,
    ::testing::Values(
        std::make_pair(RUN_COMMAND_REQUEST_TYPE, std::vector<std::string>{ RUN_COMMAND_COUNT }),
        std::make_pair(UPLOAD_FILE_REQUEST_TYPE, std::vector<std::string>{ UPLOAD_FILE_COUNT }),
        std::make_pair(UPLOAD_FOLDER_REQUEST_TYPE, std::vector<std::string>{ UPLOAD_FOLDER_COUNT }),
        std::make_pair(DOWNLOAD_FILE_REQUEST_TYPE, std::vector<std::string>{ DOWNLOAD_FILE_COUNT })));

TEST_P(TestSuccessTelemetry, Success_Telemetry_Increments_Action)
{
    auto [requestType, expectCountFieldName] = GetParam();

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [&]()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });
    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);

    actionRunner.runAction("action", "correlationid", requestType, 1);
    while (actionRunner.getIsRunning())
    {
    }
    auto out = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    std::stringstream expected;
    expected << R"({")" << expectCountFieldName.front() << R"(":1})";
    EXPECT_EQ(out, expected.str());
}

class TestFailureTelemetry : public TestTelemetryParameterized
{
};

INSTANTIATE_TEST_SUITE_P(
    TestActionRunner,
    TestFailureTelemetry,
    ::testing::Values(
        std::make_pair(
            RUN_COMMAND_REQUEST_TYPE,
            std::vector<std::string>{ RUN_COMMAND_COUNT, RUN_COMMAND_FAILED_COUNT }),
        std::make_pair(
            UPLOAD_FILE_REQUEST_TYPE,
            std::vector<std::string>{ UPLOAD_FILE_COUNT, UPLOAD_FILE_FAILED_COUNT }),
        std::make_pair(
            UPLOAD_FOLDER_REQUEST_TYPE,
            std::vector<std::string>{ UPLOAD_FOLDER_COUNT, UPLOAD_FOLDER_FAILED_COUNT }),
        std::make_pair(
            DOWNLOAD_FILE_REQUEST_TYPE,
            std::vector<std::string>{ DOWNLOAD_FILE_COUNT, DOWNLOAD_FILE_FAILED_COUNT })));

TEST_P(TestFailureTelemetry, Failed_Telemetry_Increments_Action)
{
    auto [requestType, requestCountFieldName] = GetParam();

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);

    actionRunner.runAction("action", "correlationid", requestType, 1);
    while (actionRunner.getIsRunning())
    {
    }
    auto out = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    std::stringstream expected;
    expected << "{";
    for (const auto& field : requestCountFieldName)
    {
        expected << R"(")" << field << R"(":1,)";
    }
    expected.seekp(-1, std::ios_base::end);
    expected << "}";

    EXPECT_EQ(out, expected.str());
}

class TestTimeoutTelemetry : public TestTelemetryParameterized
{
};

INSTANTIATE_TEST_SUITE_P(
    TestActionRunner,
    TestTimeoutTelemetry,
    ::testing::Values(
        std::make_pair(
            RUN_COMMAND_REQUEST_TYPE,
            std::vector<std::string>{ RUN_COMMAND_COUNT, RUN_COMMAND_FAILED_COUNT, RUN_COMMAND_TIMEOUT_COUNT }),
        std::make_pair(
            UPLOAD_FILE_REQUEST_TYPE,
            std::vector<std::string>{ UPLOAD_FILE_COUNT, UPLOAD_FILE_FAILED_COUNT, UPLOAD_FILE_TIMEOUT_COUNT }),
        std::make_pair(
            UPLOAD_FOLDER_REQUEST_TYPE,
            std::vector<std::string>{ UPLOAD_FOLDER_COUNT, UPLOAD_FOLDER_FAILED_COUNT, UPLOAD_FOLDER_TIMEOUT_COUNT }),
        std::make_pair(
            DOWNLOAD_FILE_REQUEST_TYPE,
            std::vector<std::string>{ DOWNLOAD_FILE_COUNT, DOWNLOAD_FILE_FAILED_COUNT, DOWNLOAD_FILE_TIMEOUT_COUNT })));

TEST_P(TestTimeoutTelemetry, Timeout_Telemetry_Increments_Action)
{
    auto [requestType, requestCountFieldName] = GetParam();

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_, "sophos-spl-user", "sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_, _)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, writeFile(_, _)).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_, _)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _))
                .WillOnce(Return(Common::Process::ProcessStatus::TIMEOUT))
                .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, sendSIGUSR1()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(2));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);

    actionRunner.runAction("action", "correlationid", requestType, 1);
    while (actionRunner.getIsRunning())
    {
    }
    auto out = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    std::stringstream expected;
    expected << "{";
    for (const auto& field : requestCountFieldName)
    {
        expected << R"(")" << field << R"(":1,)";
    }
    expected.seekp(-1, std::ios_base::end);
    expected << "}";

    EXPECT_EQ(out, expected.str());
}

TEST_P(TestTimeoutTelemetry, Hang_Telemetry_Increments_Action)
{
    auto [requestType, requestCountFieldName] = GetParam();

    auto mockIFilePermissions = std::make_unique<MockFilePermissions>();
    EXPECT_CALL(*mockIFilePermissions, chown(_, "sophos-spl-user", "sophos-spl-group")).Times(1);
    EXPECT_CALL(*mockIFilePermissions, chmod(_, _)).Times(1);
    Tests::replaceFilePermissions(std::move(mockIFilePermissions));

    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, writeFile(_, _)).Times(1);
    EXPECT_CALL(*filesystemMock, moveFile(_, _)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _))
                .WillOnce(Return(Common::Process::ProcessStatus::TIMEOUT))
                .WillOnce(Return(Common::Process::ProcessStatus::TIMEOUT));
            EXPECT_CALL(*mockProcess, sendSIGUSR1()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill()).Times(1);
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(9));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);

    actionRunner.runAction("action", "correlationid", requestType, 1);
    while (actionRunner.getIsRunning())
    {
    }
    auto out = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    std::stringstream expected;
    expected << "{";
    for (const auto& field : requestCountFieldName)
    {
        expected << R"(")" << field << R"(":1,)";
    }
    expected.seekp(-1, std::ios_base::end);
    expected << "}";

    EXPECT_EQ(out, expected.str());
}

class TestExpiredTelemetry : public TestTelemetryParameterized
{
};

INSTANTIATE_TEST_SUITE_P(
    TestActionRunner,
    TestExpiredTelemetry,
    ::testing::Values(
        std::make_pair(
            RUN_COMMAND_REQUEST_TYPE,
            std::vector<std::string>{ RUN_COMMAND_COUNT, RUN_COMMAND_EXPIRED_COUNT, RUN_COMMAND_FAILED_COUNT }),
        std::make_pair(
            UPLOAD_FILE_REQUEST_TYPE,
            std::vector<std::string>{ UPLOAD_FILE_COUNT, UPLOAD_FILE_EXPIRED_COUNT, UPLOAD_FILE_FAILED_COUNT }),
        std::make_pair(
            UPLOAD_FOLDER_REQUEST_TYPE,
            std::vector<std::string>{ UPLOAD_FOLDER_COUNT, UPLOAD_FOLDER_EXPIRED_COUNT, UPLOAD_FOLDER_FAILED_COUNT }),
        std::make_pair(
            DOWNLOAD_FILE_REQUEST_TYPE,
            std::vector<std::string>{ DOWNLOAD_FILE_COUNT, DOWNLOAD_FILE_EXPIRED_COUNT, DOWNLOAD_FILE_FAILED_COUNT })));

TEST_P(TestExpiredTelemetry, Expired_Telemetry_Increments_Action)
{
    auto [requestType, requestCountFieldName] = GetParam();

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(_, _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("some output"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(4));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    auto queueTask = std::make_shared<TaskQueue>();
    ActionRunner actionRunner(queueTask);

    actionRunner.runAction("action", "correlationid", requestType, 1);
    while (actionRunner.getIsRunning())
    {
    }
    auto out = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    std::stringstream expected;
    expected << "{";
    for (const auto& field : requestCountFieldName)
    {
        expected << R"(")" << field << R"(":1,)";
    }
    expected.seekp(-1, std::ios_base::end);
    expected << "}";

    EXPECT_EQ(out, expected.str());
}