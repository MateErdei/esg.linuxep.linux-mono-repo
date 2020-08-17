/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "manager/scheduler/ScheduledScanConfiguration.h"
#include "manager/scheduler/ScanScheduler.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <tests/common/LogInitializedTests.h>

using namespace manager::scheduler;

namespace
{
    class FakeScanCompletion : public IScanComplete
    {
    public:
        void processScanComplete(std::string& scanCompletedXml) override
        {
            m_xml = scanCompletedXml;
        }
        std::string m_xml;
    };

    class TestScanScheduler : public LogInitializedTests
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

    // Redirect sderr to buffer
    std::stringstream buffer;
    std::streambuf *sbuf = std::cerr.rdbuf();
    std::cerr.rdbuf(buffer.rdbuf());

    int count = 0;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        count++;
    } while(buffer.str().find("INFO Completed scan Scan Now") == std::string::npos && count < 200);

    EXPECT_THAT(buffer.str(), testing::HasSubstr("INFO Starting scan Scan Now"));
    EXPECT_THAT(buffer.str(), testing::HasSubstr("INFO Completed scan Scan Now"));

    scheduler.requestStop();
    scheduler.join();

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