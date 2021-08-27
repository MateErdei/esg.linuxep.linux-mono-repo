/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Helpers/LogInitializedTests.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/pluginimpl/OsqueryLogIngest.h>

#include <gtest/gtest.h>
#include <modules/pluginimpl/TelemetryConsts.h>

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

TEST_F(OsqueryTelemetryTests, memoryUtilExceedLimitTriggersMTRTelemetryIncrement) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "osquery extension /opt/sophos-spl/plugins/edr/extensions/SophosMTR.ext (2297) stopping: Memory limits exceeded: 265804000\n";
    OsqueryLogIngest ingester;
    ingester(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"mtr-extension-restarts-memory\":1}");
}

TEST_F(OsqueryTelemetryTests, cpuUtilExceedLimitTriggersMTRTelemetryIncrement) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "osquery extension /opt/sophos-spl/plugins/edr/extensions/SophosMTR.ext (2297) stopping: Maximum sustainable CPU utilization limit exceeded:\n";
    OsqueryLogIngest ingester;
    ingester(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"mtr-extension-restarts-cpu\":1}");
}

TEST_F(OsqueryTelemetryTests, mtrExtensionCrashingTriggersMTRTelemetryIncrement) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "osquery extension /opt/sophos-spl/plugins/edr/extensions/SophosMTR.ext (30991) stopping: Cannot find process\n";
    OsqueryLogIngest ingester;
    ingester(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"mtr-extension-restarts\":1}");
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

TEST_F(OsqueryTelemetryTests, telemetryCountersIncrementsOnFailedXDRQuery) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "Error executing scheduled query bad_query_malformed: near \"selectts\": syntax error\n";
    OsqueryLogIngest ingester;
    ingester(line);
    ingester(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"scheduled-queries\":{\"bad_query_malformed\":{\"query-error-count\":2}}}");
}

TEST_F(OsqueryTelemetryTests, telemetryCountersHandlesNoQueryName) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "Error executing scheduled query : near \"selectts\": syntax error\n";
    OsqueryLogIngest ingester;
    ingester(line);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"scheduled-queries\":{\"\":{\"query-error-count\":1}}}");
}

TEST_F(OsqueryTelemetryTests, telemetryCountersHandlesMultipleBadQueries) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line = "Error executing scheduled query : near \"selectts\": syntax error\n";
    std::string line2 = "Error executing scheduled query bad-query: near \"selectts\": syntax error\n";
    OsqueryLogIngest ingester;
    ingester(line);
    ingester(line2);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"scheduled-queries\":{\"\":{\"query-error-count\":1},\"bad-query\":{\"query-error-count\":1}}}");
}

TEST_F(OsqueryTelemetryTests, telemetryCountersHandlesAQueryhasSucessCasesAndFailedOnce) // NOLINT
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    std::string line2 = "Error executing scheduled query example-query: near \"selectts\": syntax error\n";
    OsqueryLogIngest ingester;
    ingester(line2);
    std::string queryName = "example-query";
    std::stringstream key;
    key << plugin::telemetryScheduledQueries << "." << queryName;
    std::string scheduledQueryKey = key.str();

    telemetry.appendStat(scheduledQueryKey + "." + plugin::telemetryRecordSize, 300);
    telemetry.increment(scheduledQueryKey + "." + plugin::telemetryRecordsCount, 1L);
    telemetry.updateTelemetryWithStats();
    telemetry.updateTelemetryWithAllStdDeviationStats();

    EXPECT_EQ(telemetry.serialiseAndReset(),
            "{\"scheduled-queries\":{\"example-query\":{\"query-error-count\":1,\"record-size-avg\":300.0,"
            "\"record-size-max\":300.0,\"record-size-min\":300.0,\"record-size-std-deviation\":0.0,\"records-count\":1}}}");
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