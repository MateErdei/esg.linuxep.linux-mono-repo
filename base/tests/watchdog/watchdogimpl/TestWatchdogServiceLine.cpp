/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <watchdog/watchdogimpl/Watchdog.h>
#include <watchdog/watchdogimpl/WatchdogServiceLine.h>
#include <modules/Common/ProcessImpl/ProcessImpl.h>
#include <tests/Common/ProcessImpl/MockProcess.h>

namespace
{
    class ProcessReplacement
    {
    public:
        ProcessReplacement( std::function<std::unique_ptr<Common::Process::IProcess>()> functor)
        {
            Common::ProcessImpl::ProcessFactory::instance().replaceCreator(functor);
        }
        ~ProcessReplacement()
        {
            Common::ProcessImpl::ProcessFactory::instance().restoreCreator();
        }
    };

    class TestWatchdogServiceLine : public ::testing::Test
    {
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    public:
        Common::ZMQWrapperApi::IContextSharedPtr m_context;
        TestWatchdogServiceLine(): m_context(Common::ZMQWrapperApi::createContext())
        {
            std::string pluginname = "plugins/" + watchdog::watchdogimpl::WatchdogServiceLine::WatchdogServiceLineName + ".ipc";
            Common::ApplicationConfiguration::applicationConfiguration().setData(pluginname, "inproc://watchdogservice.ipc");

//            auto mockFileSystem = new StrictMock<MockFileSystem>();
//            std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
//            Tests::replaceFileSystem(std::move(mockIFileSystemPtr));
//
//            auto mockFilePermissions = new StrictMock<MockFilePermissions>();
//            std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
//                    std::unique_ptr<MockFilePermissions>(mockFilePermissions);
//            Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
//
//            EXPECT_CALL(*mockFilePermissions, chmod(_, _)).WillRepeatedly(Return());
//            EXPECT_CALL(*mockFilePermissions, chown(_, _, _)).WillRepeatedly(Return());
        }
        ~TestWatchdogServiceLine()
        {
//            Tests::restoreFilePermissions();
//            Tests::restoreFileSystem();
        }
    };

} // namespace

TEST_F(TestWatchdogServiceLine, Construction) // NOLINT
{

    EXPECT_NO_THROW(watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context)); // NOLINT
}

TEST_F(TestWatchdogServiceLine, requestUpdateServiceThrowsExceptionIfNotWatchdogServiceIsAvailable) // NOLINT
{
    EXPECT_THROW(watchdog::watchdogimpl::WatchdogServiceLine::requestUpdateService(*m_context), watchdog::watchdogimpl::WatchdogServiceException);
}


TEST_F(TestWatchdogServiceLine, requestUpdateServiceWillIndirectlyTriggerSophosSplUpdate) // NOLINT
{

    ProcessReplacement processReplacement([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{"start", "sophos-spl-update"};
        EXPECT_CALL(*mockProcess, exec(HasSubstr("systemctl"), args, _)).Times(1);
        EXPECT_CALL(*mockProcess, waitUntilProcessEnds());
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("service triggered"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context);
    watchdog::watchdogimpl::WatchdogServiceLine::requestUpdateService(*m_context);
}

TEST_F(TestWatchdogServiceLine, requestUpdateServiceWillThrowExceptionIfSophosUpdateFails) // NOLINT
{

    ProcessReplacement processReplacement([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{"start", "sophos-spl-update"};
        EXPECT_CALL(*mockProcess, exec(HasSubstr("systemctl"), args, _)).Times(1);
        EXPECT_CALL(*mockProcess, waitUntilProcessEnds());
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("failed to trigger service"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context);
    EXPECT_THROW(watchdog::watchdogimpl::WatchdogServiceLine::requestUpdateService(*m_context), watchdog::watchdogimpl::UpdateServiceReportError);
}
