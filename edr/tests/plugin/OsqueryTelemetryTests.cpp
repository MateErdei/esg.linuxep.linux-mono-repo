/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/pluginimpl/OsqueryLogIngest.h>

#include <gtest/gtest.h>

TEST(OsqueryTelemtryTests, cpuUtilExceedLimitTriggersTelemetryIncrement) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "stopping: Maximum sustainable CPU utilization limit exceeded:\n";
    OsqueryLogIngest ingester;
    ingester(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"osquery-restarts-cpu\":1}");
}

TEST(OsqueryTelemtryTests, memoryUtilExceedLimitTriggersTelemetryIncrement) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "stopping: Memory limits exceeded:\n";
    OsqueryLogIngest ingester;
    ingester(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"osquery-restarts-memory\":1}");
}

TEST(OsqueryTelemtryTests, cpuAndMemoryLogLineTriggerTelemetryIncrement) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    OsqueryLogIngest ingester;

    std::string cpuLine = "stopping: Maximum sustainable CPU utilization limit exceeded:\n";
    ingester(cpuLine);

    std::string memoryLine = "stopping: Memory limits exceeded:\n";
    ingester(memoryLine);

    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"osquery-restarts-cpu\":1,\"osquery-restarts-memory\":1}");
}

TEST(OsqueryTelemtryTests, telemetryCountersNotIncrementedOnUnrecognisedLogLine) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "this is not recognised\n";
    OsqueryLogIngest ingester;
    ingester(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{}");
}