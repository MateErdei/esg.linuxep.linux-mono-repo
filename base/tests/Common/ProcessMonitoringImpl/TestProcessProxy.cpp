/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Process/IProcessInfo.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <Common/ProcessMonitoring/IProcessProxy.h>
#include <Common/ProcessMonitoringImpl/ProcessProxy.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FakeTimeUtils.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/ProcessImpl/MockProcess.h>

namespace
{
    class TestProcessProxy : public ::testing::Test
    {
    public:
        ~TestProcessProxy()
        {
            Common::ProcessImpl::ProcessFactory::instance().restoreCreator();
        }

    private:
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    };
} // namespace

TEST_F(TestProcessProxy, createProcessProxyDoesntThrow) // NOLINT
{
    auto processInfoPtr = Common::Process::createEmptyProcessInfo();
    ASSERT_NO_THROW(Common::ProcessMonitoring::createProcessProxy(std::move(processInfoPtr)));
}

TEST_F(TestProcessProxy, WontStartPluginWithoutExecutable) // NOLINT
{
    testing::internal::CaptureStderr();
    auto info = Common::Process::createEmptyProcessInfo();

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay.count(), 3600);
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("Not starting plugin without executable"));
}

TEST_F(TestProcessProxy, WontStartPluginIfExecutableDoesNotExist) // NOLINT
{
    testing::internal::CaptureStderr();
    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    const std::string execPath = "./foobar";
    std::string fullPath = Common::FileSystem::join(INST, execPath);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1));
    std::string logMessage = testing::internal::GetCapturedStderr();
    std::string errorMessage = "Executable does not exist at : " + fullPath;
    EXPECT_THAT(logMessage, ::testing::HasSubstr(errorMessage.c_str()));
}

TEST_F(TestProcessProxy, WontStartPluginIfExecutableGroupUserNameIsUnset) // NOLINT
{
    testing::internal::CaptureStderr();
    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    const std::string execPath = "./foobar";
    std::string fullPath = Common::FileSystem::join(INST, execPath);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1));
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Not starting plugin: invalid user name or group name"));
}

TEST_F(TestProcessProxy, WillStartPluginWithExecutable) // NOLINT
{
    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    const std::string execPath = "./foobar";
    const std::string absolutePath = INST + "/foobar";
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([absolutePath]() {
        std::vector<std::string> args;
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(absolutePath, args, _, _, _)).Times(1);
        EXPECT_CALL(*mockProcess, kill(2)).Times(1); // In the destructor of PluginProxy
        EXPECT_CALL(*mockProcess, setOutputLimit(_)).Times(1);
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1));
}

TEST_F(TestProcessProxy, WillWaitAfterExitBeforeRestartingPlugin) // NOLINT
{
    testing::internal::CaptureStderr();

    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

    const std::string execPath = "./foobar";
    const std::string absolutePath = INST + "/foobar";
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([absolutePath]() {
        std::vector<std::string> args;
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(absolutePath, args, _, _, _)).Times(1);
        EXPECT_CALL(*mockProcess, setOutputLimit(_)).Times(1);
        EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1));

    EXPECT_NO_THROW(proxy.checkForExit()); // NOLINT
    delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::seconds(10)); // Not starting for 10 seconds

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errStd, ::testing::HasSubstr("/opt/sophos-spl/foobar exited"));
}

TEST_F(TestProcessProxy, checkExpectedExitIsNotLogged) // NOLINT
{
    testing::internal::CaptureStderr();

    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

    const std::string execPath = "./foobar";
    const std::string absolutePath = INST + "/foobar";
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([absolutePath]() {
        std::vector<std::string> args;
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(absolutePath, args, _, _, _)).Times(1);
        EXPECT_CALL(*mockProcess, setOutputLimit(_)).Times(1);
        EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        EXPECT_CALL(*mockProcess, kill(2)).WillOnce(Return(false));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1));

    proxy.setEnabled(false);
    proxy.ensureStateMatchesOptions();

    EXPECT_NO_THROW(proxy.checkForExit()); // NOLINT
    delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1)); // process has exited, so we

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errStd, ::testing::Not(::testing::HasSubstr("/opt/sophos-spl/foobar exited")));
}

TEST_F(TestProcessProxy, checkDoesNotReportErrorWhenItIssuesKill) // NOLINT
{
    testing::internal::CaptureStderr();

    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

    const std::string execPath = "./foobar";
    const std::string absolutePath = INST + "/foobar";
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([absolutePath]() {
        std::vector<std::string> args;
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(absolutePath, args, _, _, _)).Times(1);
        EXPECT_CALL(*mockProcess, setOutputLimit(_)).Times(1);
        EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(ECANCELED));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        EXPECT_CALL(*mockProcess, kill(2)).WillOnce(Return(false));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
        new SequenceOfFakeTime({ 10, 10 + 3 }, std::chrono::milliseconds(0), []() {})));

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1));

    proxy.setEnabled(false);
    proxy.ensureStateMatchesOptions();

    EXPECT_NO_THROW(proxy.checkForExit()); // NOLINT
    delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1)); // process has exited, so we

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errStd, ::testing::Not(::testing::HasSubstr("/opt/sophos-spl/foobar exited")));
    EXPECT_THAT(errStd, ::testing::Not(::testing::HasSubstr("died with")));
    EXPECT_THAT(errStd, ::testing::HasSubstr("killed after waiting for 3 seconds"));
}

TEST_F(TestProcessProxy, checkItDoesReportErrorWhenItDidNotIssueKill) // NOLINT
{
    testing::internal::CaptureStderr();

    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

    const std::string execPath = "./foobar";
    const std::string absolutePath = INST + "/foobar";
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([absolutePath]() {
        std::vector<std::string> args;
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(absolutePath, args, _, _, _)).Times(1);
        EXPECT_CALL(*mockProcess, setOutputLimit(_)).Times(1);
        EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(ECANCELED));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
        new SequenceOfFakeTime({ 10, 10 + 3 }, std::chrono::milliseconds(0), []() {})));

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1));

    EXPECT_NO_THROW(proxy.checkForExit()); // NOLINT

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errStd, ::testing::HasSubstr("was forcefully stopped"));
}

TEST_F(TestProcessProxy, checkItDoesReportDiedWithWhenIsDifferentCode) // NOLINT
{
    testing::internal::CaptureStderr();

    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

    const std::string execPath = "./foobar";
    const std::string absolutePath = INST + "/foobar";
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([absolutePath]() {
        std::vector<std::string> args;
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(absolutePath, args, _, _, _)).Times(1);
        EXPECT_CALL(*mockProcess, setOutputLimit(_)).Times(1);
        EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(50));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
        new SequenceOfFakeTime({ 10, 10 + 3 }, std::chrono::milliseconds(0), []() {})));

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1));

    EXPECT_NO_THROW(proxy.checkForExit()); // NOLINT

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errStd, ::testing::HasSubstr("died with "));
}
