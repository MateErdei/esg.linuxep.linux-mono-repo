/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/Diagnose/diagnose/SystemCommands.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/ProcessImpl/MockProcess.h>

#include <regex>

using ::testing::Return;

namespace
{
    const std::string L_dfTLocalLines("Filesystem     Type     1K-blocks     Used Available Use% Mounted on\n"
                                      "udev           ひらがな    4041952        0   4041952   0% /dev\n"
                                      "tmpfs          tmpfs       814448     1544    812904   1% /run\n"
                                      "/dev/sda1      ext4      41020640 34176708   4730500  88% /\n"
                                      "tmpfs          tmpfs      4072224    35724   4036500   1% /dev/shm\n");
} // namespace

class DiagnoseSystemCommandsTests : public ::testing::Test
{
public:
    void TearDown() override { Common::ProcessImpl::ProcessFactory::instance().restoreCreator(); }

    void setupMocks(size_t numberOfMockProcesses)
    {
        ASSERT_TRUE(numberOfMockProcesses > 0);
        m_mockProcessIndex = 0;

        for (size_t i = 0; i < numberOfMockProcesses; ++i)
        {
            m_mockProcesses.emplace_back(new MockProcess());
        }

        Common::ProcessImpl::ProcessFactory::instance().replaceCreator([this]() {
            return (m_mockProcessIndex == m_mockProcesses.size()) ? nullptr
                                                                  : std::move(m_mockProcesses[m_mockProcessIndex++]);
        });

        m_mockFileSystem = new StrictMock<MockFileSystem>();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr(m_mockFileSystem);
        Tests::replaceFileSystem(std::move(mockIFileSystemPtr));
    }

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    std::vector<std::unique_ptr<MockProcess>> m_mockProcesses;

    StrictMock<MockFileSystem>* m_mockFileSystem;

private:
    size_t m_mockProcessIndex = 0;
};

TEST_F(DiagnoseSystemCommandsTests, RunCommandRunAndWritesToFileOk) // NOLINT
{
    setupMocks(1);
    auto& mockProcess = m_mockProcesses[0];

    std::string systemDirPath("/Never/Createdir/");
    diagnose::SystemCommands systemCommands(systemDirPath);
    EXPECT_CALL(*m_mockFileSystem, isExecutable(_)).WillRepeatedly(Return(true));

    EXPECT_CALL(*mockProcess, exec(_, _));
    EXPECT_CALL(*mockProcess, setOutputLimit(_));
    EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(L_dfTLocalLines));
    EXPECT_CALL(*mockProcess, exitCode()).WillRepeatedly(Return(EXIT_SUCCESS));
    // write to file called with output
    EXPECT_CALL(*m_mockFileSystem, writeFile((systemDirPath + "df"), L_dfTLocalLines));

    auto retCode = systemCommands.runCommand("df", { "-h" }, "df");

    ASSERT_EQ(retCode, EXIT_SUCCESS);
}

TEST_F(DiagnoseSystemCommandsTests, RunCommandHandlesProcessExitCodeIsFailure) // NOLINT
{
    setupMocks(1);
    auto& mockProcess = m_mockProcesses[0];

    std::string systemDirPath("/Never/Createdir/");
    diagnose::SystemCommands systemCommands(systemDirPath);
    EXPECT_CALL(*m_mockFileSystem, isExecutable(_)).WillRepeatedly(Return(true));

    EXPECT_CALL(*mockProcess, exec(_, _));
    EXPECT_CALL(*mockProcess, setOutputLimit(_));
    EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return("Command failed with unexpected blabhla"));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(2));

    // write to file called with error message
    EXPECT_CALL(*m_mockFileSystem, writeFile((systemDirPath + "df"), _));

    // implied assert no throw
    auto retCode = systemCommands.runCommand("df", { "-h" }, "df");
    ASSERT_EQ(retCode, EXIT_FAILURE);
}

TEST_F(DiagnoseSystemCommandsTests, RunCommandHandlesProcessGetExecutablePathException) // NOLINT
{
    setupMocks(1);

    std::string systemDirPath("/Never/Createdir/");
    std::string executableName("df");
    diagnose::SystemCommands systemCommands(systemDirPath);
    EXPECT_CALL(*m_mockFileSystem, isExecutable(_)).WillRepeatedly(Return(false));

    // write to file called with error message
    EXPECT_CALL(
        *m_mockFileSystem,
        writeFile((systemDirPath + executableName), "Executable " + executableName + " is not installed."));

    // implied assert no throw
    auto retCode = systemCommands.runCommand(executableName, { "-h" }, "df");
    ASSERT_EQ(retCode, EXIT_FAILURE);
}

TEST_F(DiagnoseSystemCommandsTests, RunCommandMultipleTimesWithTimeout) // NOLINT
{
    setupMocks(2);
    std::string systemDirPath("/Never/Createdir/");
    diagnose::SystemCommands systemCommands(systemDirPath);

    InSequence s;
    { // mockProcesses_[0]
        EXPECT_CALL(*m_mockFileSystem, isExecutable(_)).WillRepeatedly(Return(true));

        EXPECT_CALL(*m_mockProcesses[0], setOutputLimit(_));
        EXPECT_CALL(*m_mockProcesses[0], exec(_, _));
        EXPECT_CALL(*m_mockProcesses[0], wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::TIMEOUT));
        EXPECT_CALL(*m_mockProcesses[0], kill());
        EXPECT_CALL(*m_mockProcesses[0], output()).WillOnce(Return(L_dfTLocalLines));

        // write to file called with timeout as reason for failure
        std::string outputToFile(L_dfTLocalLines + "***End Of Command Output***\n");
        std::string timeoutError(
            "Running command failed to complete with error: Timed out after 10s while running: '/usr/bin/df -h'");
        EXPECT_CALL(*m_mockFileSystem, writeFile(systemDirPath + "df-timeout", outputToFile + timeoutError));
    }

    auto retCodeFail = systemCommands.runCommand("df", { "-h" }, "df-timeout");
    ASSERT_EQ(retCodeFail, EXIT_FAILURE);

    { // mockProcesses_[1]
        EXPECT_CALL(*m_mockFileSystem, isExecutable(_)).WillRepeatedly(Return(true));

        EXPECT_CALL(*m_mockProcesses[1], setOutputLimit(_));
        EXPECT_CALL(*m_mockProcesses[1], exec(_, _));
        EXPECT_CALL(*m_mockProcesses[1], wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*m_mockProcesses[1], output()).WillOnce(Return(L_dfTLocalLines));
        EXPECT_CALL(*m_mockProcesses[1], exitCode()).WillRepeatedly(Return(EXIT_SUCCESS));
        // write to file called with output
        EXPECT_CALL(*m_mockFileSystem, writeFile((systemDirPath + "df"), L_dfTLocalLines));
    }

    auto retCodeSuccess = systemCommands.runCommand("df", { "-h" }, "df");
    ASSERT_EQ(retCodeSuccess, EXIT_SUCCESS);
}