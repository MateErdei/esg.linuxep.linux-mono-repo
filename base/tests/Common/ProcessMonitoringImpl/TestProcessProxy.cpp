// Copyright 2019-2023 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Process/IProcessInfo.h"
#include "Common/ProcessImpl/ProcessImpl.h"
#include "Common/ProcessMonitoring/IProcessProxy.h"
#include "Common/ProcessMonitoringImpl/ProcessProxy.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "tests/Common/Helpers/FakeTimeUtils.h"
#include "tests/Common/Helpers/FilePermissionsReplaceAndRestore.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockProcess.h"

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

TEST_F(TestProcessProxy, createProcessProxyDoesntThrow)
{
    auto processInfoPtr = Common::Process::createEmptyProcessInfo();
    ASSERT_NO_THROW(Common::ProcessMonitoring::createProcessProxy(std::move(processInfoPtr)));
}

TEST_F(TestProcessProxy, WontStartPluginIfExecutableDoesNotExist)
{
    testing::internal::CaptureStderr();
    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    const std::string execPath = "./foobar";
    std::string fullPath = Common::FileSystem::join(INST, execPath);

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(false));
    auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::seconds(10));
    std::string logMessage = testing::internal::GetCapturedStderr();
    std::string errorMessage = "Executable does not exist at : " + fullPath;
    EXPECT_THAT(logMessage, ::testing::HasSubstr(errorMessage.c_str()));
}

TEST_F(TestProcessProxy, WontStartPluginIfExecutableGroupUserNameIsUnset)
{
    testing::internal::CaptureStderr();
    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    const std::string execPath = "./foobar";
    std::string fullPath = Common::FileSystem::join(INST, execPath);

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::seconds(10));
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Not starting plugin: invalid user name or group name"));
}

TEST_F(TestProcessProxy, WontStartPluginIfExecutableGroupUserNameNoLongerValid)
{
    testing::internal::CaptureStderr();
    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    const std::string execPath = "./foobar";
    std::string fullPath = Common::FileSystem::join(INST, execPath);

    auto mockFilePermissions = new NiceMock<MockFilePermissions>();
    std::unique_ptr<Tests::ScopedReplaceFilePermissions> scopedReplaceFilePermissions =
        std::make_unique<Tests::ScopedReplaceFilePermissions>(std::unique_ptr<Common::FileSystem::IFilePermissions>(mockFilePermissions));

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();

    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));

    std::pair<uid_t, gid_t> validUserAndGroupId = std::make_pair(0,0);
    std::pair<uid_t, gid_t> invalidUserAndGroupId = std::make_pair(-1,-1);

    EXPECT_CALL(*mockFilePermissions, getUserAndGroupId("root")).WillOnce(Return(invalidUserAndGroupId));
    EXPECT_CALL(*mockFilePermissions, getUserAndGroupId("root")).WillOnce(Return(validUserAndGroupId)).RetiresOnSaturation();

    auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableFullPath(execPath);
    // Setup will return and store valid user and group id's
    info->setExecutableUserAndGroup("root");


    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));

    // On ensureStateMatchesOptions will result in the UserId and Group Id being re-evaluated and result in invalid user name and group.
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::seconds(10));
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Not starting plugin: invalid user name or group name"));
}

TEST_F(TestProcessProxy, WillStartPluginWithExecutable)
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

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

    auto info = Common::Process::createEmptyProcessInfo();
    info-> setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::seconds(10));
}

TEST_F(TestProcessProxy, WillWaitAfterExitBeforeRestartingPlugin)
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

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::seconds(10));

    EXPECT_NO_THROW(proxy.checkForExit());
    delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::seconds(20)); // Not starting for 20 seconds

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errStd, ::testing::HasSubstr("/opt/sophos-spl/foobar exited"));
}

TEST_F(TestProcessProxy, checkExpectedExitIsNotLogged)
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

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::seconds(10));

    proxy.setEnabled(false);
    proxy.ensureStateMatchesOptions();

    EXPECT_NO_THROW(proxy.checkForExit());
    delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1)); // process has exited, so we

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errStd, ::testing::Not(::testing::HasSubstr("/opt/sophos-spl/foobar exited")));
}

TEST_F(TestProcessProxy, checkDoesNotReportErrorWhenItIssuesKill)
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

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
        new SequenceOfFakeTime({ 10, 10 + 3 }, std::chrono::milliseconds(0), []() {})));

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::seconds(10));

    proxy.setEnabled(false);
    proxy.ensureStateMatchesOptions();

    EXPECT_NO_THROW(proxy.checkForExit());
    delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1)); // process has exited, so we

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errStd, ::testing::Not(::testing::HasSubstr("/opt/sophos-spl/foobar exited")));
    EXPECT_THAT(errStd, ::testing::Not(::testing::HasSubstr("died with")));
    EXPECT_THAT(errStd, ::testing::HasSubstr("killed after waiting for 3 seconds"));
}

TEST_F(TestProcessProxy, checkItDoesReportErrorWhenItDidNotIssueKill)
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

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
        new SequenceOfFakeTime({ 10, 10 + 3 }, std::chrono::milliseconds(0), []() {})));

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::seconds(10));

    EXPECT_NO_THROW(proxy.checkForExit());

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errStd, ::testing::HasSubstr("was forcefully stopped"));
}

TEST_F(TestProcessProxy, checkItDoesReportDiedWithWhenIsDifferentCode)
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
        EXPECT_CALL(*mockProcess, nativeExitCode()).WillOnce(Return(50));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
        new SequenceOfFakeTime({ 10, 10 + 3 }, std::chrono::milliseconds(0), []() {})));

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::seconds(10));

    EXPECT_NO_THROW(proxy.checkForExit());

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errStd, ::testing::HasSubstr("died with "));
}

TEST_F(TestProcessProxy, checkBackoffIncreasesCorrectly)
{
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(false));
    auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

    auto info = Common::Process::createEmptyProcessInfo();

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));

    std::chrono::seconds delay;
    for (unsigned int i = 0; i < 9; i++)
    {
        delay = proxy.ensureStateMatchesOptions();
        EXPECT_EQ(delay.count(), proxy.m_minimumBackoff.count() * std::pow(2.0, i));
    }
    // Note: doing proxy.ensureStateMatchesOptions() any more will end up hitting the maximum backoff limit
}

TEST_F(TestProcessProxy, checkBackoffGetsLimitedCorrectly)
{
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(false));
    auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

    auto info = Common::Process::createEmptyProcessInfo();

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));

    std::chrono::seconds delay;
    for (unsigned int i = 0; i < 9; i++)
    {
        delay = proxy.ensureStateMatchesOptions();
    }
    // Should hit backoff limit when ensureStateMatchesOptions() runs now

    // Check a few times to make sure backoff is limited
    for (unsigned int i = 0; i < 3; i++)
    {
        delay = proxy.ensureStateMatchesOptions();
        EXPECT_EQ(delay, proxy.m_maximumBackoff);
    }
}

TEST_F(TestProcessProxy, checkBackoffCanBeResetCorrectly)
{
    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    const std::string execPath = "./foobar";

    auto info = Common::Process::createEmptyProcessInfo();
    info->setExecutableUserAndGroup("root:root");
    info->setExecutableFullPath(execPath);

    Common::ProcessMonitoringImpl::ProcessProxy proxy(std::move(info));

    unsigned int numBuildupRuns = 5;
    {
        auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
        EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(false));
        auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

        std::chrono::seconds delay;
        for (unsigned int i = 0; i < numBuildupRuns; i++) {
            delay = proxy.ensureStateMatchesOptions();
            EXPECT_EQ(delay.count(), proxy.m_minimumBackoff.count() * std::pow(2.0, i));
        }
    }

    {
        auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
        EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
        auto scopedReplaceFileSystem = Tests::ScopedReplaceFileSystem(std::move(filesystemMock));

        // Previously when the start() function was running in ensureStateMatchesOptions(), there would be an early
        // exit as executable "did not exist". Now as isFile(_) will return true, start() gets to the end and sets
        // m_sharedState.m_running to true. The next time ensureStateMatchesOptions() runs, the backoff is reset
        std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
        EXPECT_EQ(delay.count(), proxy.m_minimumBackoff.count() * std::pow(2.0, numBuildupRuns));

        delay = proxy.ensureStateMatchesOptions();
        EXPECT_EQ(delay.count(), proxy.m_maximumBackoff.count());
    }
}