/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "datatypes/sophos_filesystem.h"
#include "manager/scheduler/ScanScheduler.h"
#include "manager/scheduler/ScheduledScanConfiguration.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <tests/common/MemoryAppender.h>

#include <fstream>
#include <utility>

namespace fs = sophos_filesystem;
using namespace manager::scheduler;
using namespace std::chrono_literals;

namespace
{
    class FakeScanCompletion : public IScanComplete
    {
    public:
        void processScanComplete(std::string& scanCompletedXml, int exitCode) override
        {
            m_xml = scanCompletedXml;
            m_count++;
            m_exitCode = exitCode;
        }
        std::string m_xml;
        int m_count = 0;
        int m_exitCode;
    };

    constexpr char LOG_CATEGORY[] = "ScanScheduler";
    class TestScanScheduler : public MemoryAppenderUsingTestsTemplate<LOG_CATEGORY>
    {
    protected:
        Common::XmlUtilities::AttributesMap m_scheduledScanPolicy = Common::XmlUtilities::parseXml(
                R"MULTILINE(<?xml version="1.0"?>
                            <config xmlns="http://www.sophos.com/EE/EESavConfiguration">
                              <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
                              <onDemandScan>
                                <scanSet>
                                  <!-- if {{scheduledScanEnabled}} -->
                                  <scan>
                                    <name>Another scan!</name>
                                    <schedule>
                                      <daySet>
                                        <!-- for day in {{scheduledScanDays}} -->
                                        <day>friday</day>
                                      </daySet>
                                      <timeSet>
                                        <time>20:00:00</time>
                                      </timeSet>
                                    </schedule>
                                   </scan>
                                </scanSet>
                                <fileReputation>{{fileReputationCollectionDuringOnDemandScan}}</fileReputation>
                              </onDemandScan>
                            </config>
                            )MULTILINE");

        Common::XmlUtilities::AttributesMap m_emptyPolicy = Common::XmlUtilities::parseXml(
                R"MULTILINE(<?xml version="1.0"?>
                            <config xmlns="http://www.sophos.com/EE/EESavConfiguration">
                              <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
                            </config>
                            )MULTILINE");

        fs::path m_basePath;

        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_basePath = fs::temp_directory_path();
            m_basePath /= test_info->test_case_name();
            m_basePath /= test_info->name();
            fs::remove_all(m_basePath);
            fs::create_directories(m_basePath);
            fs::create_directory(m_basePath / "var");

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", m_basePath);
        }
        void TearDown() override
        {
            fs::remove_all(m_basePath);
        }
    };


    class FakeFileWalker
    {
    public:
        FakeFileWalker(const fs::path& basePath)
        {
            fs::path scanLauncherDir = basePath / "sbin";
            fs::create_directories(scanLauncherDir);
            m_scanLauncherPath = scanLauncherDir / "scheduled_file_walker_launcher";

            fs::path logDir = basePath / "log";
            fs::create_directories(logDir);
            m_logPath = logDir / "scan.log";

            fs::path varDir = basePath / "var";
            fs::create_directories(varDir);
            m_flagPath = logDir / "scan.flag";

            std::ofstream(m_flagPath).close();

            create_fake_file_walker();
        }

        ~FakeFileWalker()
        {
            stopScan();
        }

        void stopScan()
        {
            fs::remove(m_flagPath);
        }

        std::vector<std::string> readLog()
        {
            std::ifstream file(m_logPath);
            if (file.bad())
            {
                return std::vector<std::string>{};
            }

            std::vector<std::string> lines;
            std::string line;
            while (std::getline(file, line))
            {
                lines.push_back(line);
            }
            return lines;
        }

    private:
        void create_fake_file_walker()
        {
            std::ofstream scanLauncherFs(m_scanLauncherPath);
            scanLauncherFs << "#!/bin/bash\n"
                           << "echo \"$$ - $(date) - $@\" >> " << m_logPath << '\n'
                           << "for ((x=100;x>0;x--))\n"
                           << "do\n"
                           << "  [[ -f " << m_flagPath << " ]] || exit 0\n"
                           << "  sleep 0.01\n"
                           << "done\n"
                           << "exit 1\n";
            scanLauncherFs.close();
            fs::permissions(m_scanLauncherPath, fs::perms::owner_all | fs::perms::group_all);
        }

        fs::path m_scanLauncherPath;
        fs::path m_logPath;
        fs::path m_flagPath;

    };

    class WeirdTimeScanScheduler : public ScanScheduler
    {
    public:
        using ScanScheduler::ScanScheduler;

    protected:
        /**
         * Get the 'current' time, allow to be overridden by unit tests
         * @return
         */
        time_t get_current_time(bool findNextTime) override
        {
            if (findNextTime)
            {
                return 0;
            }
            return 60*60*24*7+1;
        }

        time_t calculate_delay(time_t, time_t) override
        {
            return 60;
        }
    };

    /*
     * Simple thread runner, used to ensure threads are stopped before objects are destroyed.
     */
    class SimpleThreadRunner
    {
    public:
        explicit SimpleThreadRunner(Common::Threads::AbstractThread& thread)
            : m_thread(thread)
        {
            m_thread.start();
        }

        ~SimpleThreadRunner()
        {
            killThreads();
        }

        void killThreads()
        {
            m_thread.requestStop();
            m_thread.join();
        }

    private:
        Common::Threads::AbstractThread& m_thread;
    };
}

TEST_F(TestScanScheduler, constructor) //NOLINT
{
    FakeScanCompletion scanCompletion;
    ScanScheduler scheduler { scanCompletion };
    ScheduledScanConfiguration scheduledScanConfiguration(m_emptyPolicy);
    scheduler.updateConfig(scheduledScanConfiguration);
}

TEST_F(TestScanScheduler, destructWhileRunning) //NOLINT
{
    // ensure that we don't destroy ScanScheduler before stopping its thread
    FakeScanCompletion scanCompletion;
    ScanScheduler scheduler { scanCompletion };
    SimpleThreadRunner schedulerThread(scheduler);
}

TEST_F(TestScanScheduler, findNextTime) //NOLINT
{
    FakeScanCompletion scanCompletion;
    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_scheduledScanPolicy);
    scheduler.updateConfig(scheduledScanConfiguration);

    struct timespec spec{};
    ASSERT_NO_THROW(scheduler.findNextTime(spec));
    // next time interval should never be more than an hour
    EXPECT_EQ(spec.tv_nsec, 0);
    EXPECT_GT(spec.tv_sec, 0);
    EXPECT_LE(spec.tv_sec, 3600);
}

TEST_F(TestScanScheduler, findNextTimeNoScanConfigured) //NOLINT
{
    FakeScanCompletion scanCompletion;
    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_emptyPolicy);
    scheduler.updateConfig(scheduledScanConfiguration);

    struct timespec spec{};
    ASSERT_NO_THROW(scheduler.findNextTime(spec));
    EXPECT_EQ(spec.tv_nsec, 0);
    EXPECT_EQ(spec.tv_sec, 3600);
}

TEST_F(TestScanScheduler, scanNow) //NOLINT
{
    FakeScanCompletion scanCompletion;
    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_emptyPolicy);

    SimpleThreadRunner schedulerThread(scheduler);

    scheduler.updateConfig(scheduledScanConfiguration);

    UsingMemoryAppender holder(*this);

    scheduler.scanNow();

    ASSERT_TRUE(waitForLog("Evaluating Scan Now", 500ms));
    ASSERT_TRUE(waitForLog("Starting scan Scan Now", 500ms));
    ASSERT_TRUE(waitForLog("Completed scan Scan Now with exit code:", 500ms));
    // Check that we no longer log the output from the scan process as it can be found in the scan log
    EXPECT_FALSE(appenderContains("Output:"));

    schedulerThread.killThreads();

    EXPECT_EQ(scanCompletion.m_count, 1);
}


TEST_F(TestScanScheduler, scanNowTwice) //NOLINT
{
    FakeScanCompletion scanCompletion;
    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_emptyPolicy);

    SimpleThreadRunner schedulerThread(scheduler);

    scheduler.updateConfig(scheduledScanConfiguration);

    UsingMemoryAppender holder(*this);

    scheduler.scanNow();

    ASSERT_TRUE(waitForLog("Evaluating Scan Now", 500ms));
    ASSERT_TRUE(waitForLog("Starting scan Scan Now", 500ms));
    ASSERT_TRUE(waitForLog("Completed scan Scan Now", 500ms));

    ASSERT_TRUE(waitForLog("Exiting scan thread", 500ms));

    clearMemoryAppender();

    scheduler.scanNow();

    ASSERT_TRUE(waitForLog("Evaluating Scan Now", 500ms));
    EXPECT_TRUE(waitForLog("Starting scan Scan Now", 500ms));
    EXPECT_FALSE(appenderContains("Refusing to run a second Scan named: Scan Now"));
    EXPECT_TRUE(waitForLog("Completed scan Scan Now", 500ms));

    schedulerThread.killThreads();
}


TEST_F(TestScanScheduler, scanNow_refusesConcurrentScanNow) //NOLINT
{
    UsingMemoryAppender holder(*this);

    FakeFileWalker fileWalker(m_basePath);

    FakeScanCompletion scanCompletion;

    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_emptyPolicy);

    SimpleThreadRunner schedulerThread(scheduler);

    scheduler.updateConfig(scheduledScanConfiguration);
    scheduler.scanNow();
    ASSERT_TRUE(waitForLog("Starting scan Scan Now", 500ms));

    scheduler.scanNow();
    EXPECT_TRUE(waitForLog("Refusing to run a second Scan named: Scan Now", 500ms));

    fileWalker.stopScan();
    ASSERT_TRUE(waitForLog("Completed scan Scan Now", 500ms));

    schedulerThread.killThreads();

    EXPECT_EQ(scanCompletion.m_count, 1);

    // Check the actual script only ran once
    auto log_lines = fileWalker.readLog();
    EXPECT_THAT(log_lines, testing::SizeIs(1));
}

TEST_F(TestScanScheduler, scanNowIncrementsTelemetryCounter) //NOLINT
{
    UsingMemoryAppender holder(*this);

    // Clear any pending telemetry
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();

    FakeScanCompletion scanCompletion;

    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_emptyPolicy);

    SimpleThreadRunner schedulerThread(scheduler);

    scheduler.updateConfig(scheduledScanConfiguration);
    scheduler.scanNow();

    ASSERT_TRUE(waitForLog("Evaluating Scan Now", 500ms));

    schedulerThread.killThreads();

    telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    std::string ExpectedTelemetry{ R"sophos({"scan-now-count":1})sophos" };
    EXPECT_EQ(telemetryResult, ExpectedTelemetry);

}

TEST_F(TestScanScheduler, runsScheduledScan) //NOLINT
{
    UsingMemoryAppender holder(*this);

    fs::path pluginInstall = m_basePath;
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", pluginInstall);

    FakeFileWalker fileWalker(m_basePath);

    FakeScanCompletion scanCompletion;

    WeirdTimeScanScheduler scheduler{scanCompletion};

    // configure before running to avoid racing on setting/getting m_config
    ScheduledScanConfiguration scheduledScanConfiguration(m_scheduledScanPolicy);
    scheduler.updateConfig(scheduledScanConfiguration);

    SimpleThreadRunner schedulerThread(scheduler);

    ASSERT_TRUE(waitForLog("Starting scan Another scan!",  1500ms));

    fileWalker.stopScan();
    EXPECT_TRUE(waitForLog("Completed scan Another scan!", 1000ms));

    schedulerThread.killThreads();

    EXPECT_EQ(scanCompletion.m_count, 1);

    // Check the actual script only ran once
    auto log_lines = fileWalker.readLog();
    EXPECT_THAT(log_lines, testing::SizeIs(1));
}

TEST_F(TestScanScheduler, runsScheduledScanAndScanNow) //NOLINT
{
    UsingMemoryAppender holder(*this);

    fs::path pluginInstall = m_basePath;
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", pluginInstall);

    FakeFileWalker fileWalker(m_basePath);

    FakeScanCompletion scanCompletion;

    WeirdTimeScanScheduler scheduler{scanCompletion};

    // configure before running to avoid racing on setting/getting m_config
    ScheduledScanConfiguration scheduledScanConfiguration(m_scheduledScanPolicy);
    scheduler.updateConfig(scheduledScanConfiguration);

    SimpleThreadRunner schedulerThread(scheduler);

    scheduler.scanNow();

    ASSERT_TRUE(waitForLog("Starting scan Another scan!", 1500ms));
    ASSERT_TRUE(waitForLog("Starting scan Scan Now", 500ms));

    fileWalker.stopScan();
    ASSERT_TRUE(waitForLog("Completed scan Another scan!", 1000ms));
    ASSERT_TRUE(waitForLog("Completed scan Scan Now", 500ms));

    schedulerThread.killThreads();

    EXPECT_EQ(scanCompletion.m_count, 2);

    // Check the actual script only ran twice
    auto log_lines = fileWalker.readLog();
    EXPECT_THAT(log_lines, testing::SizeIs(2));
}
