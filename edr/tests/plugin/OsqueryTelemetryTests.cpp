/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <modules/pluginimpl/OsqueryProcessImpl.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

TEST(OsqueryTelemtryTests, cpuUtilExceedLimitTriggersTelemetryIncrement) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "stopping: Maximum sustainable CPU utilization limit exceeded:";
    Plugin::processOsqueryLogLineForTelemetry(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"osquery-restarts-cpu\":1}");
}

TEST(OsqueryTelemtryTests, memoryUtilExceedLimitTriggersTelemetryIncrement) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "stopping: Memory limits exceeded:";
    Plugin::processOsqueryLogLineForTelemetry(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"osquery-restarts-memory\":1}");
}

TEST(OsqueryTelemtryTests, cpuAndMemoryLogLineTriggerTelemetryIncrement) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string cpuLine = "stopping: Maximum sustainable CPU utilization limit exceeded:";
    Plugin::processOsqueryLogLineForTelemetry(cpuLine);

    std::string memoryLine = "stopping: Memory limits exceeded:";
    Plugin::processOsqueryLogLineForTelemetry(memoryLine);

    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"osquery-restarts-cpu\":1,\"osquery-restarts-memory\":1}");
}

TEST(OsqueryTelemtryTests, telemetryCountersNotIncrementedOnUnrecognisedLogLine) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "this is not recognised";
    Plugin::processOsqueryLogLineForTelemetry(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{}");
}