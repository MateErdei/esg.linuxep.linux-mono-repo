/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>

#include "datatypes/sophos_filesystem.h"
#include "modules/manager/scanprocessmonitor/ScanProcessMonitor.h"
#include "ScanProcessMonitorMemoryAppenderUsingTests.h"
#include "tests/datatypes/MockSysCalls.h"

#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Threads/AbstractThread.h>

#include <chrono>
#include <fstream>

using namespace plugin::manager::scanprocessmonitor;
using ScanProcessMonitorPtr = std::unique_ptr<ScanProcessMonitor>;
namespace fs = sophos_filesystem;

namespace
{
    class TestScanProcessMonitor : public ScanProcessMonitorMemoryAppenderUsingTests
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
            m_mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
            m_mockSystemCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
            m_systemCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();

            Tests::ScopedReplaceFileSystem replacer(std::move(m_mockFileSystem));
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        fs::path m_testDir;
        std::unique_ptr<StrictMock<MockFileSystem>> m_mockFileSystem;
        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSystemCallWrapper;
        datatypes::ISystemCallWrapperSharedPtr m_systemCallWrapper;
    };
}

TEST_F(TestScanProcessMonitor, constructionWithExistingPath) // NOLINT
{
    EXPECT_NO_THROW(auto m = std::make_unique<ScanProcessMonitor>("", m_systemCallWrapper));
}

TEST_F(TestScanProcessMonitor, createConfigMonitor) // NOLINT
{
    Common::Threads::NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper);
}

TEST_F(TestScanProcessMonitor, runConfigMonitor) // NOLINT
{
    Common::Threads::NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();
    a.requestStop();
    a.join();
}


using steady_clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

bool waitForPipe(Common::Threads::NotifyPipe& expected, steady_clock::duration wait_time)
{
    auto deadline = steady_clock::now() + wait_time;
    do
    {
        if (expected.notified())
        {
            return true;
        }
        std::this_thread::sleep_for(10ms);
    } while (steady_clock::now() < deadline);
    return false;
}

static inline std::string toString(const fs::path& p)
{
    return p.string();
}

const static auto MONITOR_LATENCY = 100ms; //NOLINT

TEST_F(TestScanProcessMonitor, ConfigMonitorIsNotifiedOfWrite) // NOLINT
{
    std::ofstream ofs("hosts");
    ofs << "This is some text";
    ofs.close();

    Common::Threads::NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    ofs.open("hosts");
    ofs << "This is some different text";
    ofs.close();

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestScanProcessMonitor, ConfigMonitorFail) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    Common::Threads::NotifyPipe configPipe;
    EXPECT_CALL(*m_mockSystemCallWrapper, pselect(_, _, _, _, _, _)).WillOnce(Return(-1));
    ConfigMonitor a(configPipe, m_mockSystemCallWrapper, m_testDir);
    a.start();
    ASSERT_TRUE(waitForLog("failure in ConfigMonitor: pselect failed: "));
    a.requestStop();
    a.join();
}

TEST_F(TestScanProcessMonitor, ConfigMonitorLogsErrorWhenPselectFails) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    Common::Threads::NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_mockSystemCallWrapper, m_testDir);
    EXPECT_CALL(*m_mockSystemCallWrapper, pselect(_, _, _, _, _, _)).WillOnce(DoAll(InvokeWithoutArgs(&a, &Common::Threads::AbstractThread::requestStop), Return(-1)));
    a.start();
    EXPECT_FALSE(appenderContains("failure in ConfigMonitor: pselect failed: "));
    a.join();
}

TEST_F(TestScanProcessMonitor, ConfigMonitorDoesNotLogErrorWhenShuttingDownAndPselectFails) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::ofstream ofs("hosts");
    ofs << "This is some text";
    ofs.close();

    Common::Threads::NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    ofs.open("hosts");
    ofs << "This is some text";
    ofs.close();

    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));
    EXPECT_TRUE(appenderContains("System configuration not changed for "));
    EXPECT_FALSE(appenderContains("System configuration updated for "));

    a.requestStop();
    a.join();
}

TEST_F(TestScanProcessMonitor, ConfigMonitorIsNotifiedOfAnotherWrite) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);


    std::ofstream ofs("hosts");
    ofs << "This is some text";
    ofs.close();

    Common::Threads::NotifyPipe configPipe;
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


TEST_F(TestScanProcessMonitor, ConfigMonitorIsNotNotifiedOnCreateOutsideDir) // NOLINT
{
    Common::Threads::NotifyPipe configPipe;

    fs::create_directory("watched");
    fs::create_directory("not_watched");

    ConfigMonitor a(configPipe, m_systemCallWrapper, toString(m_testDir / "watched"));
    a.start();

    std::ofstream ofs("not_watched/hosts");
    ofs.close();

    EXPECT_FALSE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestScanProcessMonitor, ConfigMonitorIsNotifiedOfMove) // NOLINT
{
    Common::Threads::NotifyPipe configPipe;

    fs::create_directory("watched");
    fs::create_directory("not_watched");

    ConfigMonitor a(configPipe, m_systemCallWrapper, toString(m_testDir / "watched"));
    a.start();

    std::ofstream ofs("not_watched/hosts");
    ofs << "This is some text";
    ofs.close();

    fs::rename("not_watched/hosts", "watched/hosts");

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestScanProcessMonitor, ConfigMonitorIsNotifiedOfWriteToSymlinkTarget) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::path symlinkTargetDir = m_testDir / "targetDir";
    fs::path symlinkTarget = symlinkTargetDir / "targetFile";
    fs::create_directories(symlinkTargetDir);

    std::ofstream ofs(symlinkTarget);
    ofs << "This is some text";
    ofs.close();

    fs::create_symlink(symlinkTarget, "hosts");

    Common::Threads::NotifyPipe configPipe;
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

TEST_F(TestScanProcessMonitor, ConfigMonitorIsNotifiedOfAnotherWriteToSymlinkTarget) // NOLINT
{
    fs::path symlinkTargetDir = m_testDir / "targetDir";
    fs::path symlinkTarget = symlinkTargetDir / "targetFile";
    fs::create_directories(symlinkTargetDir);

    std::ofstream ofs(symlinkTarget);
    ofs << "This is some text";
    ofs.close();

    fs::create_symlink(symlinkTarget, "hosts");

    Common::Threads::NotifyPipe configPipe;
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

TEST_F(TestScanProcessMonitor, ConfigMonitorIsNotifiedOfWriteToRelativeSymlinkTarget) // NOLINT
{
    fs::path symlinkTargetDir = m_testDir / "targetDir";
    fs::path symlinkTarget = symlinkTargetDir / "targetFile";
    fs::create_directories(symlinkTargetDir);

    std::ofstream ofs(symlinkTarget);
    ofs << "This is some text";
    ofs.close();

    fs::create_symlink("targetDir/targetFile", "hosts");

    Common::Threads::NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    ofs.open(symlinkTarget);
    ofs << "This is some different text";
    ofs.close();

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestScanProcessMonitor, ConfigMonitorIsNotifiedOfWriteToMultipleSymlinkTarget) // NOLINT
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

    Common::Threads::NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);
    a.start();

    ofs.open(symlinkTarget);
    ofs << "This is some different text";
    ofs.close();

    EXPECT_TRUE(waitForPipe(configPipe, MONITOR_LATENCY));

    a.requestStop();
    a.join();
}

TEST_F(TestScanProcessMonitor, ConfigMonitorHandlesRecursiveSymlink) // NOLINT
{
    fs::create_symlink("hosts", m_testDir / "hosts");

    Common::Threads::NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, m_testDir);

    a.start();
    a.requestStop();
    a.join();
}

TEST_F(TestScanProcessMonitor, ConfigMonitorHandlesInvalidDir) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::path invalidDir = m_testDir / "invalidDir";

    Common::Threads::NotifyPipe configPipe;
    ConfigMonitor a(configPipe, m_systemCallWrapper, invalidDir);

    a.start();

    EXPECT_TRUE(waitForLog("Failed to watch directory: \"" + invalidDir.string() + "\""));
    EXPECT_TRUE(waitForLog("Unable to monitor DNS config files"));
    EXPECT_TRUE(waitForLog("Failed to initialise inotify"));

    a.requestStop();
    a.join();
}

TEST_F(TestScanProcessMonitor, ConfigMonitorIsNotifiedOfChangedSymlink) // NOLINT
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

    Common::Threads::NotifyPipe configPipe;
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

TEST_F(TestScanProcessMonitor, ConfigMonitorIgnoresChangedSymlinkSameContent) // NOLINT
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

    Common::Threads::NotifyPipe configPipe;
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