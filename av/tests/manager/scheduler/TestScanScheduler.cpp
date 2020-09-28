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

    static constexpr char LOG_CATEGORY[] = "ScanScheduler";
    class TestScanScheduler : public MemoryAppenderUsingTestsTemplate<LOG_CATEGORY>
    {
        public:
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
    };
}

TEST_F(TestScanScheduler, scanNow) //NOLINT
{
    FakeScanCompletion scanCompletion;

    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_emptyPolicy);

    scheduler.start();

    scheduler.updateConfig(scheduledScanConfiguration);
    scheduler.scanNow();

    // Redirect stderr to buffer
    std::stringstream buffer;
    std::streambuf *sbuf = std::cerr.rdbuf();
    std::cerr.rdbuf(buffer.rdbuf());

    int count = 0;
    while(buffer.str().find("Evaluating Scan Now") == std::string::npos && count < 200)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        count++;
    }

    EXPECT_THAT(buffer.str(), testing::HasSubstr("Starting scan Scan Now"));
    EXPECT_THAT(buffer.str(), testing::HasSubstr("Completed scan Scan Now"));

    scheduler.requestStop();
    scheduler.join();

    // Reset stderr
    std::cerr.rdbuf(sbuf);
}

TEST_F(TestScanScheduler, scanNow_refusesSecondScanNow) //NOLINT
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
    std::ofstream scanLauncherFs(scanLauncherPath);
    scanLauncherFs << "#!/bin/bash\n";
    scanLauncherFs << "sleep 1\n";
    scanLauncherFs << "echo $$ >> " << log_file << '\n';
    scanLauncherFs.close();
    fs::permissions(scanLauncherPath, fs::perms::owner_all | fs::perms::group_all);

    FakeScanCompletion scanCompletion;

    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_emptyPolicy);

    scheduler.start();

    scheduler.updateConfig(scheduledScanConfiguration);
    scheduler.scanNow();
    ASSERT_TRUE(waitForLog("Starting scan Scan Now", 1000));
    scheduler.scanNow();

    ASSERT_TRUE(waitForLog("Completed scan Scan Now", 2000000));
    EXPECT_TRUE(waitForLog("Refusing to run a second Scan named: Scan Now", 2000000));

    scheduler.requestStop();
    scheduler.join();

    EXPECT_EQ(scanCompletion.m_count, 1);

    // Check the actual script only ran once
    std::ifstream read_log_file(log_file);
    ASSERT_TRUE(read_log_file.good());
    int lines = 0;
    std::string line;
    while (std::getline(read_log_file, line))
    {
        lines++;
    }
    EXPECT_EQ(lines, 1);
}

TEST_F(TestScanScheduler, scanNowIncrementsTelemetryCounter) //NOLINT
{
    FakeScanCompletion scanCompletion;

    ScanScheduler scheduler{scanCompletion};

    ScheduledScanConfiguration scheduledScanConfiguration(m_emptyPolicy);

    scheduler.start();

    scheduler.updateConfig(scheduledScanConfiguration);
    scheduler.scanNow();

    // Redirect sderr to buffer
    std::stringstream buffer;
    std::streambuf *sbuf = std::cerr.rdbuf();
    std::cerr.rdbuf(buffer.rdbuf());

    int count = 0;
    while(buffer.str().find("INFO Starting Scan Now") == std::string::npos && count < 200)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        count++;
    }

    scheduler.requestStop();
    scheduler.join();

    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    std::string ExpectedTelemetry{ R"sophos({"scan-now-count":1})sophos" };
    EXPECT_EQ(telemetryResult, ExpectedTelemetry);

    // Reset stderr
    std::cerr.rdbuf(sbuf);
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