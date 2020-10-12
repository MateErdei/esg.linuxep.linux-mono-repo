/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

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

namespace fs = sophos_filesystem;
using namespace manager::scheduler;

namespace
{
    class FakeScanCompletion : public IScanComplete
    {
    public:
        void processScanComplete(std::string& scanCompletedXml) override
        {
            m_xml = scanCompletedXml;
            m_count++;
        }
        std::string m_xml;
        int m_count = 0;
    };

    constexpr char LOG_CATEGORY[] = "ScanScheduler";
    class TestScanScheduler : public MemoryAppenderUsingTestsTemplate<LOG_CATEGORY>
    {
        public:
            TestScanScheduler();


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

        fs::path m_pluginInstall;
    };
    TestScanScheduler::TestScanScheduler()
        : m_pluginInstall("/tmp/TestScanScheduler")
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData("PLUGIN_INSTALL", m_pluginInstall);

    }

    int count_lines(std::ifstream& file)
    {
        if (file.bad())
        {
            return 0;
        }
        int lines = 0;
        std::string line;
        while (std::getline(file, line))
        {
            lines++;
        }
        return lines;
    }

    int count_lines(const std::string& filename)
    {
        std::ifstream file(filename);
        EXPECT_TRUE(file.good());
        return count_lines(file);
    }

    void create_fake_file_walker(const fs::path& scanLauncherPath, const fs::path& log_file)
    {
        std::ofstream scanLauncherFs(scanLauncherPath);
        scanLauncherFs << "#!/bin/bash\n";
        scanLauncherFs << "sleep 0.6\n";
        scanLauncherFs << "echo $$ >> " << log_file << '\n';
        scanLauncherFs.close();
        fs::permissions(scanLauncherPath, fs::perms::owner_all | fs::perms::group_all);
    }

    class WithFakeFileWalker
    {
    public:
        fs::path m_scanLauncherPath;
        WithFakeFileWalker(const fs::path& scanLauncherPath, const fs::path& log_file)
            : m_scanLauncherPath(scanLauncherPath)
        {
            create_fake_file_walker(scanLauncherPath, log_file);
        }
        ~WithFakeFileWalker()
        {
            ::unlink(m_scanLauncherPath.c_str());
        }
    };
}

TEST_F(TestScanScheduler, findNextTime) //NOLINT
{
    FakeScanCompletion scanCompletion;

    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_scheduledScanPolicy);
    scheduler.updateConfig(scheduledScanConfiguration);

    struct timespec spec{};
    ASSERT_NO_THROW(scheduler.findNextTime(spec));
    ASSERT_EQ(spec.tv_nsec, 0);
    ASSERT_EQ(spec.tv_sec, 3600);
}

TEST_F(TestScanScheduler, scanNow) //NOLINT
{
    FakeScanCompletion scanCompletion;

    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_emptyPolicy);

    scheduler.start();

    scheduler.updateConfig(scheduledScanConfiguration);

    UsingMemoryAppender holder(*this);

    scheduler.scanNow();

    ASSERT_TRUE(waitForLog("Evaluating Scan Now", 1000));
    ASSERT_TRUE(waitForLog("Starting scan Scan Now", 1000));
    ASSERT_TRUE(waitForLog("Completed scan Scan Now", 1000));

    scheduler.requestStop();
    scheduler.join();
}

TEST_F(TestScanScheduler, scanNow_refusesSecondScanNow) //NOLINT
{
    UsingMemoryAppender holder(*this);

    fs::path scanLauncherDir = m_pluginInstall / "sbin";
    fs::create_directories(scanLauncherDir);

    fs::path log_file = m_pluginInstall / "scan.log";
    ::unlink(log_file.c_str());

    fs::path scanLauncherPath = scanLauncherDir / "scheduled_file_walker_launcher";
    WithFakeFileWalker fakeFileWalker(scanLauncherPath, log_file);

    FakeScanCompletion scanCompletion;

    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_emptyPolicy);

    scheduler.start();

    scheduler.updateConfig(scheduledScanConfiguration);
    scheduler.scanNow();
    ASSERT_TRUE(waitForLog("Starting scan Scan Now", 100000));
    scheduler.scanNow();

    ASSERT_TRUE(waitForLog("Completed scan Scan Now", 200000));
    EXPECT_TRUE(waitForLog("Refusing to run a second Scan named: Scan Now", 200000));

    scheduler.requestStop();
    scheduler.join();

    EXPECT_EQ(scanCompletion.m_count, 1);

    // Check the actual script only ran once
    std::ifstream read_log_file(log_file);
    ASSERT_TRUE(read_log_file.good());
    int lines = count_lines(read_log_file);
    EXPECT_EQ(lines, 1);
}

TEST_F(TestScanScheduler, scanNowIncrementsTelemetryCounter) //NOLINT
{
    UsingMemoryAppender holder(*this);

    // Clear any pending telemetry
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();

    FakeScanCompletion scanCompletion;

    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_emptyPolicy);

    scheduler.start();

    scheduler.updateConfig(scheduledScanConfiguration);
    scheduler.scanNow();

    ASSERT_TRUE(waitForLog("Evaluating Scan Now", 10000));

    scheduler.requestStop();
    scheduler.join();

    telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    std::string ExpectedTelemetry{ R"sophos({"scan-now-count":1})sophos" };
    EXPECT_EQ(telemetryResult, ExpectedTelemetry);

}

namespace
{
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
            return 1;
        }
    };

}

TEST_F(TestScanScheduler, runsScheduledScan) //NOLINT
{
    UsingMemoryAppender holder(*this);

    fs::path pluginInstall = "/tmp/TestScanScheduler";
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", pluginInstall);
    fs::path scanLauncherDir = pluginInstall / "sbin";
    fs::create_directories(scanLauncherDir);

    fs::path log_file = pluginInstall / "scan.log";
    ::unlink(log_file.c_str());

    fs::path scanLauncherPath = scanLauncherDir / "scheduled_file_walker_launcher";
    WithFakeFileWalker fakeFileWalker(scanLauncherPath, log_file);

    FakeScanCompletion scanCompletion;

    WeirdTimeScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_scheduledScanPolicy);

    scheduler.start();

    scheduler.updateConfig(scheduledScanConfiguration);
    EXPECT_TRUE(waitForLog("Starting scan Another scan!",  100000)); // micro-seconds
    EXPECT_TRUE(waitForLog("Completed scan Another scan!", 200000)); // micro-seconds

    scheduler.requestStop();
    scheduler.join();

    EXPECT_EQ(scanCompletion.m_count, 1);

    // Check the actual script only ran once
    int lines = count_lines(log_file);
    EXPECT_EQ(lines, 1);
}

TEST_F(TestScanScheduler, runsScheduledScanAndScanNow) //NOLINT
{
    UsingMemoryAppender holder(*this);

    fs::path pluginInstall = "/tmp/TestScanScheduler";
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", pluginInstall);
    fs::path scanLauncherDir = pluginInstall / "sbin";
    fs::create_directories(scanLauncherDir);

    fs::path log_file = pluginInstall / "scan.log";
    ::unlink(log_file.c_str());

    fs::path scanLauncherPath = scanLauncherDir / "scheduled_file_walker_launcher";
    WithFakeFileWalker fakeFileWalker(scanLauncherPath, log_file);

    FakeScanCompletion scanCompletion;

    WeirdTimeScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_scheduledScanPolicy);

    scheduler.start();
    scheduler.updateConfig(scheduledScanConfiguration);
    scheduler.scanNow();
    ASSERT_TRUE(waitForLog("Starting scan Another scan!", 10000));
    ASSERT_TRUE(waitForLog("Starting scan Scan Now", 10000));
    ASSERT_TRUE(waitForLog("Completed scan Another scan!", 200000));
    ASSERT_TRUE(waitForLog("Completed scan Scan Now", 200000));

    scheduler.requestStop();
    scheduler.join();

    EXPECT_EQ(scanCompletion.m_count, 2);

    // Check the actual script only ran once
    int lines = count_lines(log_file);
    EXPECT_EQ(lines, 2);
}
