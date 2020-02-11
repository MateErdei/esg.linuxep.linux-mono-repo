/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "manager/scheduler/ScheduledScanConfiguration.h"
#include "manager/scheduler/ScanScheduler.h"

#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>

using namespace manager::scheduler;

TEST(TestScanScheduler, scanNow) //NOLINT
{
    ScanScheduler scheduler;
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
</config>
)MULTILINE");

    ScheduledScanConfiguration scheduledScanConfiguration(attributeMap);

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
    } while(buffer.str().find("INFO Completed scan scanNow") == std::string::npos && count < 200);

    EXPECT_THAT(buffer.str(), testing::HasSubstr("INFO Starting scan scanNow"));
    EXPECT_THAT(buffer.str(), testing::HasSubstr("INFO Completed scan scanNow"));

    scheduler.requestStop();
    scheduler.join();

    // Reset stderr
    std::cerr.rdbuf(sbuf);
}
