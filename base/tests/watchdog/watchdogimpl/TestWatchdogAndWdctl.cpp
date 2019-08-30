/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#ifndef ARTISANBUILD
#include "gtest/gtest.h"

#include <tests/Common/Helpers/TempDir.h>
#include <watchdog/watchdogimpl/Watchdog.h>
#include <modules/Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <modules/Common/UtilityImpl/StringUtils.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <modules/wdctl/wdctlimpl/wdctl_bootstrap.h>
#include <gmock/gmock.h>
#include <thread>

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
  "executableFullPath": "/bin/sleep",
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
    return Common::UtilityImpl::StringUtils::orderedStringReplace(templ,{{"@@SOPHOS_INSTALL@@", installdir},{"@@user@@",user},{"@@group@@", group}} );
}


class TestWatchdogAndWdctl : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

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
    Common::Logging::ConsoleLoggingSetup consoleLogging;
    ScopedFilePermission scopedFilePermission;
};
static void *watchdogmain(void * )
{
    static int result =0;
    watchdog::watchdogimpl::Watchdog m;
    result =  m.initialiseAndRun();
    return &result;
}

class WatchdogRunner
{
public:
    WatchdogRunner():m_thread_id{}
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
    ~WatchdogRunner()
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

void TestWatchdogAndWdctl::SetUpTestCase()
{
    tempDir = Tests::TempDir::makeTempDir();
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, installDir());
    Common::ApplicationConfiguration::applicationConfiguration().setData("watchdog.ipc", "ipc://" + installDir()+"/watchdog.ipc");
    tempDir->createFile("base/etc/logger.conf", "VERBOSITY=DEBUG\n");
    tempDir->createFile("base/pluginRegistry/fakeplugin.json", templateFakePlugin(installDir()));
    std::vector<std::string> relativePaths{ {"var/ipc"}};
    tempDir->makeDirs(relativePaths);
}

void TestWatchdogAndWdctl::TearDownTestCase()
{
    tempDir.reset(nullptr);
}

TEST_F(TestWatchdogAndWdctl, WdctlIssuesStopToWatchdog) // NOLINT
{
    testing::internal::CaptureStderr();
    WatchdogRunner watchdogRunner;
    watchdogRunner.start();
    waitPluginStarted();
    ASSERT_EQ(wdctl.main_afterLogConfigured(stopArgs), 0);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("stop fakeplugin"));
    EXPECT_EQ(watchdogRunner.stop(), 0);
}

TEST_F(TestWatchdogAndWdctl, WdctlIsRunningDetectCanDetectStatusOfPlugins) // NOLINT
{
    WatchdogRunner watchdogRunner;
    watchdogRunner.start();
    waitPluginStarted();

    wdctl::wdctlimpl::wdctl_bootstrap wdctl;
    EXPECT_EQ(wdctl.main_afterLogConfigured(isRunningArgs), 0);

    EXPECT_EQ(wdctl.main_afterLogConfigured(isRunningArgs), 0);

    // stop
    EXPECT_EQ(wdctl.main_afterLogConfigured(stopArgs), 0);
    EXPECT_EQ(wdctl.main_afterLogConfigured(isRunningArgs), 1);

    EXPECT_EQ(watchdogRunner.stop(), 0);
}


#endif