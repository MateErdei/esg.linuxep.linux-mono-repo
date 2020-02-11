/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "manager/scheduler/ScheduledScanConfiguration.h"
#include "manager/scheduler/ScanScheduler.h"

#include <datatypes/Print.h>
#include <Common/Logging/ConsoleLoggingSetup.h>

using namespace manager::scheduler;

TEST(TestScanScheduler, scanNow)
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
    testing::internal::CaptureStderr();

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

    std::this_thread::sleep_for(std::chrono::milliseconds (10)); //5ms is actually enough

    std::string logs = testing::internal::GetCapturedStderr();

    EXPECT_NE(logs.find("INFO Starting scan scanNow"), std::string::npos);
    EXPECT_NE(logs.find("INFO Completed scan scanNow"), std::string::npos);

    scheduler.requestStop();
    scheduler.join();
}
