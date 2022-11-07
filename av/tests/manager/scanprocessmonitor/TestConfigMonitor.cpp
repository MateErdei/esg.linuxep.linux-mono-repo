// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ScanProcessMonitorMemoryAppenderUsingTests.h"

#include "common/ThreadRunner.h"
#include "common/NotifyPipeSleeper.h"
#include "datatypes/SystemCallWrapper.h"
#include "manager/scanprocessmonitor/ConfigMonitor.h"

#include "tests/datatypes/MockSysCalls.h"

#include <chrono>
#include <fstream>

#include <gtest/gtest.h>

using namespace ::testing;

namespace
{
    class TestConfigMonitor : public ScanProcessMonitorMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);
            fs::current_path(m_testDir);

            m_systemCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();

            m_mockSystemCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        fs::path m_testDir;
        std::shared_ptr<::testing::StrictMock<MockSystemCallWrapper>> m_mockSystemCallWrapper;
        datatypes::ISystemCallWrapperSharedPtr m_systemCallWrapper;
    };
}

using Common::Threads::NotifyPipe;
using plugin::manager::scanprocessmonitor::ConfigMonitor;

TEST_F(TestConfigMonitor, createConfigMonitor)
{
    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper);
}

TEST_F(TestConfigMonitor, runConfigMonitor)
{
    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();
    a.requestStop();
    a.join();
}

using steady_clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

static bool waitForPipe(NotifyPipe& expected, steady_clock::duration wait_time)
{
    common::NotifyPipeSleeper sleeper(expected);

    auto deadline = steady_clock::now() + wait_time;
    do
    {
        sleeper.stoppableSleep(deadline - steady_clock::now());
        if (expected.notified())
        {
            return true;
        }
    } while (steady_clock::now() < deadline);
    return false;
}

constexpr auto MONITOR_LATENCY = 100ms;

TEST_F(TestConfigMonitor, ConfigMonitorIsNotifiedOfWrite)
{
    std::ofstream ofs("hosts");
    ofs << "This is some text";
    ofs.close();

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    ofs.open("hosts");
    ofs << "This is some different text";
    ofs.close();

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorFail)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    NotifyPipe configPipe;
    EXPECT_CALL(*m_mockSystemCallWrapper, pselect(_, _, _, _, _, _)).WillOnce(Return(-1));
    ConfigMonitor a(configPipe, m_mockSystemCallWrapper, m_testDir);
    a.start();
    ASSERT_TRUE(waitForLog("failure in ConfigMonitor: pselect failed: "));
    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorLogsErrorWhenPselectFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_mockSystemCallWrapper, m_testDir);
    EXPECT_CALL(*m_mockSystemCallWrapper, pselect(_, _, _, _, _, _))
        .WillOnce(DoAll(InvokeWithoutArgs(&a, &Common::Threads::AbstractThread::requestStop), Return(-1)));
    a.start();
    EXPECT_FALSE(appenderContains("failure in ConfigMonitor: pselect failed: "));
    a.join();
}

TEST_F(TestConfigMonitor, noNotificationWhenSameContentsRewritten)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::ofstream ofs("hosts");
    ofs << "This is some text";
    ofs.close();

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    ofs.open("hosts");
    ofs << "This is some text";
    ofs.close();

    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));
    EXPECT_FALSE(appenderContains("System configuration updated for "));

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorIsNotifiedOfAnotherWrite)
{
    UsingMemoryAppender memoryAppenderHolder(*this);


    std::ofstream ofs("hosts");
    ofs << "This is some text";
    ofs.close();

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    ofs.open("hosts");
    ofs << "This is some different text";
    ofs.close();

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));
    EXPECT_TRUE(appenderContains("System configuration updated for "));
    EXPECT_FALSE(appenderContains("System configuration not changed for "));

    clearMemoryAppender();

    ofs.open("hosts");
    ofs << "This is some text";
    ofs.close();

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));
    EXPECT_TRUE(appenderContains("System configuration updated for "));
    EXPECT_FALSE(appenderContains("System configuration not changed for "));

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorIsNotifiedOfAnotherWriteAfterStopStart)
{
    UsingMemoryAppender memoryAppenderHolder(*this);


    std::ofstream ofs("hosts");
    ofs << "This is some text";
    ofs.close();

    NotifyPipe configPipe;
    auto a = std::make_shared<ConfigMonitor>(configPipe, m_systemCallWrapper, m_testDir);
    auto aThread = common::ThreadRunner(a, "a", true);
    EXPECT_TRUE(waitForLog("Config Monitor entering main loop"));

    ofs.open("hosts");
    ofs << "This is some different text";
    ofs.close();

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));
    EXPECT_TRUE(appenderContains("System configuration updated for "));
    EXPECT_FALSE(appenderContains("System configuration not changed for "));

    clearMemoryAppender();

    aThread.requestStopIfNotStopped();
    aThread.startIfNotStarted();
    EXPECT_TRUE(waitForLog("Config Monitor entering main loop"));

    ofs.open("hosts");
    ofs << "This is some text";
    ofs.close();

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));
    EXPECT_TRUE(appenderContains("System configuration updated for "));
    EXPECT_FALSE(appenderContains("System configuration not changed for "));
}

TEST_F(TestConfigMonitor, ConfigMonitorIsNotNotifiedOnCreateOutsideDir)
{
    NotifyPipe configPipe;

    fs::create_directory("watched");
    fs::create_directory("not_watched");

    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir / "watched");
    a.start();

    std::ofstream ofs("not_watched/hosts");
    ofs << "This is some text";
    ofs.close();

    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorIsNotifiedOfMove)
{
    NotifyPipe configPipe;

    fs::create_directory("watched");
    fs::create_directory("not_watched");

    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir / "watched");
    a.start();

    std::ofstream ofs("not_watched/hosts");
    ofs << "This is some text";
    ofs.close();

    fs::rename("not_watched/hosts", "watched/hosts");

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorIsNotifiedOfWriteToSymlinkTarget)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::path symlinkTargetDir = m_testDir / "targetDir";
    fs::path symlinkTarget = symlinkTargetDir / "targetFile";
    fs::create_directories(symlinkTargetDir);

    std::ofstream ofs(symlinkTarget);
    ofs << "This is some text";
    ofs.close();

    fs::create_symlink(symlinkTarget, "hosts");

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));

    ofs.open(symlinkTarget);
    ofs << "This is some different text";
    ofs.close();

    // we should get only one notification
    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));
    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));

    // ensure we don't keep checking config
    EXPECT_EQ(appenderCount("Change detected in monitored directory, checking config for changes"), 1);

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorIsNotifiedOfAnotherWriteToSymlinkTarget)
{
    fs::path symlinkTargetDir = m_testDir / "targetDir";
    fs::path symlinkTarget = symlinkTargetDir / "targetFile";
    fs::create_directories(symlinkTargetDir);

    std::ofstream ofs(symlinkTarget);
    ofs << "This is some text";
    ofs.close();

    fs::create_symlink(symlinkTarget, "hosts");

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    ofs.open(symlinkTarget);
    ofs << "This is some different text";
    ofs.close();

    // we should get only one notification
    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));
    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));

    ofs.open(symlinkTarget);
    ofs << "This is some text";
    ofs.close();

    // we should get only one notification
    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));
    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorIsNotifiedOfWriteToRelativeSymlinkTarget)
{
    fs::path symlinkTargetDir = m_testDir / "targetDir";
    fs::path symlinkTarget = symlinkTargetDir / "targetFile";
    fs::create_directories(symlinkTargetDir);

    std::ofstream ofs(symlinkTarget);
    ofs << "This is some text";
    ofs.close();

    fs::create_symlink("targetDir/targetFile", "hosts");

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    ofs.open(symlinkTarget);
    ofs << "This is some different text";
    ofs.close();

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorIsNotifiedOfWriteToMultipleSymlinkTarget)
{
    fs::path symlinkTargetDir = m_testDir / "targetDir";
    fs::path symlinkTarget = symlinkTargetDir / "targetFile";
    fs::path intermediateSymlinkTarget = symlinkTargetDir / "intermediateFile";
    fs::create_directories(symlinkTargetDir);

    std::ofstream ofs(symlinkTarget);
    ofs << "This is some text";
    ofs.close();

    fs::create_symlink(symlinkTarget, intermediateSymlinkTarget);
    fs::create_symlink(intermediateSymlinkTarget, m_testDir / "hosts");

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    ofs.open(symlinkTarget);
    ofs << "This is some different text";
    ofs.close();

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorHandlesRecursiveSymlink)
{
    fs::create_symlink("hosts", m_testDir / "hosts");

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);

    a.start();
    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorHandlesInvalidDir)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::path invalidDir = m_testDir / "invalidDir";

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, invalidDir);

    a.start();

    EXPECT_TRUE(waitForLog("Failed to watch directory: \"" + invalidDir.string() + "\""));
    EXPECT_TRUE(waitForLog("Unable to monitor DNS config files"));
    EXPECT_TRUE(waitForLog("Failed to initialise inotify"));

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorIsNotifiedOfChangedSymlink)
{
    fs::path symlinkTargetDir1 = m_testDir / "targetDir1";
    fs::path symlinkTarget1 = symlinkTargetDir1 / "targetFile";
    fs::create_directories(symlinkTargetDir1);

    std::ofstream ofs(symlinkTarget1);
    ofs << "This is some text";
    ofs.close();

    fs::path symlinkTargetDir2 = m_testDir / "targetDir2";
    fs::path symlinkTarget2 = symlinkTargetDir2 / "targetFile";
    fs::create_directories(symlinkTargetDir2);

    ofs.open(symlinkTarget2);
    ofs << "This is some different text";
    ofs.close();

    fs::create_symlink(symlinkTarget1, "hosts");

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));

    // replace the symlink
    fs::create_symlink(symlinkTarget2, "hosts.new");
    fs::rename("hosts.new", "hosts");

    // we should get only one notification
    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));
    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorIsNotifiedOfRemovedChangedSymlink)
{
    fs::path symlinkTargetDir1 = m_testDir / "targetDir1";
    fs::path symlinkTarget1 = symlinkTargetDir1 / "targetFile";
    fs::create_directories(symlinkTargetDir1);

    std::ofstream ofs(symlinkTarget1);
    ofs << "This is some text";
    ofs.close();

    fs::path symlinkTargetDir2 = m_testDir / "targetDir2";
    fs::path symlinkTarget2 = symlinkTargetDir2 / "targetFile";
    fs::create_directories(symlinkTargetDir2);

    ofs.open(symlinkTarget2);
    ofs << "This is some different text";
    ofs.close();

    fs::create_symlink(symlinkTarget1, "hosts");

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));

    // replace the symlink
    fs::remove("hosts");
    fs::create_symlink(symlinkTarget2, "hosts");

    // we should get only one notification
    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));
    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, ConfigMonitorIgnoresChangedSymlinkSameContent)
{
    fs::path symlinkTargetDir1 = m_testDir / "targetDir1";
    fs::path symlinkTarget1 = symlinkTargetDir1 / "targetFile";
    fs::create_directories(symlinkTargetDir1);

    std::ofstream ofs(symlinkTarget1);
    ofs << "This is some text";
    ofs.close();

    fs::path symlinkTargetDir2 = m_testDir / "targetDir2";
    fs::path symlinkTarget2 = symlinkTargetDir2 / "targetFile";
    fs::create_directories(symlinkTargetDir2);

    fs::copy(symlinkTarget1, symlinkTarget2);

    fs::create_symlink(symlinkTarget1, "hosts");

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));

    // replace the symlink
    fs::create_symlink(symlinkTarget2, "hosts.new");
    fs::rename("hosts.new", "hosts");

    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestConfigMonitor, catchIntermediateSymlinkChanges)
{
    fs::path monitoredDir = m_testDir / "etc";
    fs::create_directories(monitoredDir);
    fs::path monitoredHosts = monitoredDir / "hosts";

    fs::path intermediateDir = m_testDir / "intermediate";
    fs::create_directories(intermediateDir);
    fs::path intermediateHosts = intermediateDir / "isymlink";
    fs::path intermediateHostsNew = intermediateDir / "isymlink.new";

    fs::path targetDir1 = m_testDir / "targetDir1";
    fs::create_directories(targetDir1);
    fs::path target1 = targetDir1 / "targetFile";

    fs::path targetDir2 = m_testDir / "targetDir2";
    fs::create_directories(targetDir2);
    fs::path target2 = targetDir2 / "targetFile";

    std::ofstream ofs(target1);
    ofs << "Original Text";
    ofs.close();

    ofs.open(target2);
    ofs << "This is some different text";
    ofs.close();


    fs::create_symlink(intermediateHosts, monitoredHosts);
    fs::create_symlink(target1, intermediateHosts);

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, monitoredDir);
    a.start();

    EXPECT_FALSE(waitForPipe(configPipe, 10ms));

    // replace the symlink
    fs::create_symlink(target2, intermediateHostsNew);
    fs::rename(intermediateHostsNew, intermediateHosts);

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));

    ofs.open(target2);
    ofs << "Text version 3";
    ofs.close();

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}


TEST_F(TestConfigMonitor, catchBaseSymlinkChanges)
{
    fs::path monitoredDir = m_testDir / "etc";
    fs::create_directories(monitoredDir);
    fs::path monitoredHosts = monitoredDir / "hosts";
    fs::path monitoredHostsNew = monitoredDir / "hosts.new";

    fs::path targetDir1 = m_testDir / "targetDir1";
    fs::create_directories(targetDir1);
    fs::path target1 = targetDir1 / "targetFile";

    fs::path targetDir2 = m_testDir / "targetDir2";
    fs::create_directories(targetDir2);
    fs::path target2 = targetDir2 / "targetFile";

    std::ofstream ofs(target1);
    ofs << "Original Text";
    ofs.close();

    ofs.open(target2);
    ofs << "This is some different text";
    ofs.close();


    fs::create_symlink(target1, monitoredHosts);

    NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, monitoredDir);
    a.start();

    EXPECT_FALSE(waitForPipe(configPipe, 10ms));

    // replace the symlink
    fs::create_symlink(target2, monitoredHostsNew);
    fs::rename(monitoredHostsNew, monitoredHosts);

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));

    ofs.open(target2);
    ofs << "Text version 3";
    ofs.close();

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}