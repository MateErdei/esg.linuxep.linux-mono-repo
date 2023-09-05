// Copyright 2018-2023 Sophos Limited. All rights reserved.

#ifndef ARTISANBUILD
#    include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#    include "Common/FileSystemImpl/FilePermissionsImpl.h"
#    include "Common/UtilityImpl/StringUtils.h"
#    include "tests/Common/Helpers/FilePermissionsReplaceAndRestore.h"
#    include "tests/Common/Helpers/LogInitializedTests.h"
#    include "tests/Common/Helpers/MockFilePermissions.h"
#    include "tests/Common/Helpers/TempDir.h"
#    include "watchdog/watchdogimpl/Watchdog.h"
#    include "wdctl/wdctlimpl/wdctl_bootstrap.h"

#    include <gmock/gmock.h>
#    include <gtest/gtest.h>

#    include <grp.h>
#    include <mutex>
#    include <pwd.h>
#    include <thread>
#    include <unistd.h>

namespace
{
    std::string& sleepFullPath()
    {
        static std::vector<std::string> candidates = { { "/usr/bin/sleep" }, { "/bin/sleep" } };
        for (auto& sleeppath : candidates)
        {
            if (Common::FileSystem::fileSystem()->exists(sleeppath) &&
                !Common::FileSystem::fileSystem()->isSymlink(sleeppath))
            {
                return sleeppath;
            }
        }
        throw std::runtime_error("Could not find path of sleep");
    }

}

class NullFilePermission : public Common::FileSystem::FilePermissionsImpl
{
public:

    void chown(const Path& , const std::string& , const std::string& ) const override
    {

    }

    void chmod(const Path& , __mode_t ) const override
    {

    }
};


class ScopedFilePermission
{
public:
    ScopedFilePermission()
    {
        Tests::replaceFilePermissions( std::unique_ptr<Common::FileSystem::IFilePermissions>( new NullFilePermission()) );
    }
    ~ScopedFilePermission()
    {
         Tests::restoreFilePermissions();
    }
};



std::string templateFakePlugin(const std::string&  installdir)
{
    std::string templ{R"({
  "policyAppIds": [
    "MDR"
  ],
  "statusAppIds": [
    "MDR"
  ],
  "pluginName": "fakeplugin",
  "executableFullPath": "@sleep@",
  "executableArguments": [ "9" ],
  "environmentVariables": [
    {
      "name": "SOPHOS_INSTALL",
      "value": "@@SOPHOS_INSTALL@@"
    }
  ],
  "executableUserAndGroup": "@@user@@:@@group@@"
})"};
    NullFilePermission nullFilePermission;

  std::string user = nullFilePermission.getUserName(::getuid());
  std::string group = nullFilePermission.getGroupName(::getgid());
    return Common::UtilityImpl::StringUtils::orderedStringReplace(templ,{
            {"@sleep@", sleepFullPath()}, {"@@SOPHOS_INSTALL@@", installdir}, {"@@user@@",user}, {"@@group@@", group}
    } );
}


class TestWatchdogAndWdctl  : public LogInitializedTests
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    static std::mutex ensureNotInParallel;
    static std::unique_ptr<Tests::TempDir> tempDir;
    static std::string installDir()
    {
        return tempDir->dirPath();
    }
    TestWatchdogAndWdctl():
        stopArgs{ {"wdctl", "stop", "fakeplugin"}},
        isRunningArgs{ {"wdctl", "isrunning", "fakeplugin"}}
    {

    }
    void waitPluginStarted()
    {
        for( int i =0; i<10; i++)
        {
            if( wdctl.main_afterLogConfigured(isRunningArgs)==0)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    std::vector<std::string> stopArgs;
    std::vector<std::string> isRunningArgs;
    wdctl::wdctlimpl::wdctl_bootstrap wdctl;
    ScopedFilePermission scopedFilePermission;
};
static void *watchdogmain(void * )
{
    static int result =0;
    watchdog::watchdogimpl::Watchdog m;
    result =  m.initialiseAndRun();
    return &result;
}

class WatchdogWdctlRunner
{
public:
    WatchdogWdctlRunner():m_thread_id{}
    {
        state = 0;
    }
    void start()
    {
        state = 1;
        int s = pthread_create(&m_thread_id, NULL, watchdogmain, NULL);
        ASSERT_EQ(0, s);
    }

    int stop()
    {
        pthread_kill(m_thread_id, SIGTERM);
        void* res;
        pthread_join(m_thread_id, &res);
        int result = (*static_cast<int*>(res));
        state = 2;
        return result;
    }
    ~WatchdogWdctlRunner()
    {
        try
        {
            if ( state == 1)
            {
                stop();
            }
        }
        catch (std::exception & ex)
        {
            std::cerr << ex.what() << std::endl;
        }

    }
    private:
        pthread_t m_thread_id;
        int state;
};


std::unique_ptr<Tests::TempDir> TestWatchdogAndWdctl::tempDir;
std::mutex TestWatchdogAndWdctl::ensureNotInParallel;
void TestWatchdogAndWdctl::SetUpTestCase()
{
    tempDir = Tests::TempDir::makeTempDir();
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, installDir());
    Common::ApplicationConfiguration::applicationConfiguration().setData("watchdog.ipc", "ipc://" + installDir()+"/watchdog.ipc");
    tempDir->createFile("base/etc/logger.conf", "VERBOSITY=DEBUG\n");
    tempDir->createFile("base/pluginRegistry/fakeplugin.json", templateFakePlugin(installDir()));
    std::vector<std::string> relativePaths{ {"var/ipc/plugins"}};
    tempDir->makeDirs(relativePaths);
}

void TestWatchdogAndWdctl::TearDownTestCase()
{
    tempDir.reset(nullptr);
}

TEST_F(TestWatchdogAndWdctl, WdctlIssuesStopToWatchdog)
{
    {
        // Test uses real user on the machine to start process, so need to make sure the correct user and group ids are obtained.
        // for current user
        uid_t userId = -1;
        gid_t  groupId = -1;
        userId = ::getuid();
        groupId = ::getgid();

        auto mockFilePermissions = new StrictMock<MockFilePermissions>();
        std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
            std::unique_ptr<MockFilePermissions>(mockFilePermissions);
        Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
        EXPECT_CALL(*mockFilePermissions, getUserId(_)).WillRepeatedly(Return(userId));
        EXPECT_CALL(*mockFilePermissions, getGroupId(_)).WillRepeatedly(Return(groupId));
        std::pair userAndGroup = std::make_pair(userId,groupId);
        EXPECT_CALL(*mockFilePermissions, getUserAndGroupId(_)).WillRepeatedly(Return(userAndGroup));
        EXPECT_CALL(*mockFilePermissions, chmod(_, _)).WillRepeatedly(Return());
        EXPECT_CALL(*mockFilePermissions, getFilePermissions(_)).WillOnce(Return(0600));
        EXPECT_CALL(*mockFilePermissions, chown(A<const Path&>(), A<const std::string&>(), A<const std::string&>())).WillRepeatedly(Return());

        std::unique_lock<std::mutex> lock{ensureNotInParallel};
        testing::internal::CaptureStderr();
        WatchdogWdctlRunner watchdogRunner;
        watchdogRunner.start();
        waitPluginStarted();
        int retValue = wdctl.main_afterLogConfigured(stopArgs, false);
        std::string logMessage = testing::internal::GetCapturedStderr();
        if (retValue != 0)
        {
            // Provide more information if the test fails.
            // this will display the value returned and well as all the
            // log messages on failure.
            EXPECT_EQ(retValue, 0) << logMessage;
        }
        else
        {
            EXPECT_THAT(logMessage, ::testing::HasSubstr("stop fakeplugin"));
        }

        EXPECT_EQ(watchdogRunner.stop(), 0);
    }
}

TEST_F(TestWatchdogAndWdctl, WdctlIsRunningDetectCanDetectStatusOfPlugins) // NOLINT
{
    {
        // Test uses real user on the machine to start process, so need to make sure the correct user and group ids are obtained.
        // for current user

        uid_t userId = -1;
        gid_t  groupId = -1;
        userId = ::getuid();
        groupId = ::getgid();

        auto mockFilePermissions = new StrictMock<MockFilePermissions>();
        std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
            std::unique_ptr<MockFilePermissions>(mockFilePermissions);
        Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
        EXPECT_CALL(*mockFilePermissions, getUserId(_)).WillRepeatedly(Return(userId));
        EXPECT_CALL(*mockFilePermissions, getGroupId(_)).WillRepeatedly(Return(groupId));
        std::pair userAndGroup = std::make_pair(userId,groupId);
        EXPECT_CALL(*mockFilePermissions, getUserAndGroupId(_)).WillRepeatedly(Return(userAndGroup));
        EXPECT_CALL(*mockFilePermissions, chmod(_, _)).WillRepeatedly(Return());
        EXPECT_CALL(*mockFilePermissions, getFilePermissions(_)).WillOnce(Return(0600));
        EXPECT_CALL(*mockFilePermissions, chown(A<const Path&>(), A<const std::string&>(), A<const std::string&>())).WillRepeatedly(Return());

        std::unique_lock<std::mutex> lock{ensureNotInParallel};
        WatchdogWdctlRunner watchdogRunner;
        watchdogRunner.start();
        waitPluginStarted();

        wdctl::wdctlimpl::wdctl_bootstrap wdctl;
        EXPECT_EQ(wdctl.main_afterLogConfigured(isRunningArgs, false), 0);

        EXPECT_EQ(wdctl.main_afterLogConfigured(isRunningArgs, false), 0);

        // stop
        EXPECT_EQ(wdctl.main_afterLogConfigured(stopArgs, false), 0);
        EXPECT_EQ(wdctl.main_afterLogConfigured(isRunningArgs, false), 1);

        EXPECT_EQ(watchdogRunner.stop(), 0);
    }
}


#endif