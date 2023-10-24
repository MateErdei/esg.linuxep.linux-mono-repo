// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "pluginimpl/OsqueryLogIngest.h"
#include "EdrCommon/TelemetryConsts.h"

#ifdef SPL_BAZEL
#include "tests/Common/Helpers/LogInitializedTests.h"
#else
#include "Common/Helpers/LogInitializedTests.h"
#endif

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <gtest/gtest.h>


class OsqueryTelemetryTests : public LogOffInitializedTests{};

TEST_F(OsqueryTelemetryTests, cpuUtilExceedLimitTriggersTelemetryIncrement) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "stopping: Maximum sustainable CPU utilization limit exceeded:\n";
    OsqueryLogIngest ingester;
    ingester(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"osquery-restarts-cpu\":1}");
}

TEST_F(OsqueryTelemetryTests, memoryUtilExceedLimitTriggersTelemetryIncrement) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "stopping: Memory limits exceeded:\n";
    OsqueryLogIngest ingester;
    ingester(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"osquery-restarts-memory\":1}");
}

TEST_F(OsqueryTelemetryTests, cpuAndMemoryLogLineTriggerTelemetryIncrement) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    OsqueryLogIngest ingester;

    std::string cpuLine = "stopping: Maximum sustainable CPU utilization limit exceeded:\n";
    ingester(cpuLine);

    std::string memoryLine = "stopping: Memory limits exceeded:\n";
    ingester(memoryLine);

    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"osquery-restarts-cpu\":1,\"osquery-restarts-memory\":1}");
}

TEST_F(OsqueryTelemetryTests, telemetryCountersNotIncrementedOnUnrecognisedLogLine) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "this is not recognised\n";
    OsqueryLogIngest ingester;
    EXPECT_NO_THROW(ingester(line));
    EXPECT_EQ(telemetry.serialiseAndReset(), "{}");
}

TEST_F(OsqueryTelemetryTests, telemetryCountersHandlesEventMax) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line1 = "Expiring events for subscriber: syslog_events (overflowed limit 100000)\n";
    std::string line2 = "Expiring events for subscriber: user_events (overflowed limit 100000)\n";
    std::string line3 = "Expiring events for subscriber: socket_events \n";
    std::string line4 = "Expiring events for subscriber: process_events (overflowed limit 100000)\n";
    std::string line5 = "Expiring events for subscriber: selinux_events (overflowed limit 100000)\n";
    OsqueryLogIngest ingester;
    ingester(line1);
    ingester(line2);
    ingester(line3);
    ingester(line4);
    ingester(line5);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"reached-max-process-events\":true,\"reached-max-selinux-events\":true,\"reached-max-socket-events\":true,\"reached-max-syslog-events\":true,\"reached-max-user-events\":true}");
}

TEST_F(OsqueryTelemetryTests, telemetryEventMaxProcessorDoesNotthrow) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line1 = "Expiring events for subscriber: syslog_ (overflowed limit 100000)\n";
    std::string line2 = "Expiring events for subscriber:  (overflowed limit 100000)\n";
    std::string line3 = "Expiring events for subscriber:\n";
    OsqueryLogIngest ingester;
    ingester(line1);
    ingester(line2);
    ingester(line3);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{}");
}