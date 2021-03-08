/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Helpers/LogInitializedTests.h>
#include <modules/pluginimpl/OsqueryLogStringUtil.h>
#include <gtest/gtest.h>

class TestOsqueryLogStringUtil : public LogOffInitializedTests{};

TEST_F(TestOsqueryLogStringUtil, testProcessOsqueryLogLineForScheduledQueriesReturnsCorrectFormattedString) // NOLINT
{
    std::string line = "I0215 15:47:24.150650  8842 scheduler.cpp:102] Executing scheduled query host_sensor_heartbeat_check: SELECT";

    std::optional<std::string> actualResult = OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(line);

    EXPECT_EQ(actualResult.has_value(), true);
    EXPECT_EQ(actualResult.value(), "Executing query: host_sensor_heartbeat_check at: 15:47:24.150650");

}

TEST_F(TestOsqueryLogStringUtil, testProcessOsqueryLogLineForScheduledQueriesReturnsFalseForUninterestingStrings) // NOLINT
{
    std::string line = "I0215 15:47:24.150650  8842 scheduler.cpp:102] hello: SELECT";

    std::optional<std::string> actualResult = OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(line);

    EXPECT_EQ(actualResult.has_value(), false);
}

TEST_F(TestOsqueryLogStringUtil, testProcessOsqueryLogLineForScheduledQueriesReturnsFalseOnMalformedLine) // NOLINT
{
    std::vector<std::string> badLines{
        "",
        "   ",
        "time Executing scheduled query",
        "timeExecuting scheduledquery",
        "time Executing scheduled",
        "Executing scheduled query",
        "   Executing scheduled query   ",
        "I0215 Executing scheduled query host_sensor_heartbeat_check: SELECT"
        };

    for(auto& badLine : badLines)
    {
        std::optional<std::string> actualResult = OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(
                badLine);

        EXPECT_EQ(actualResult.has_value(), false);
    }
}

TEST_F(TestOsqueryLogStringUtil, testIsGenericLogLineReturnsTrueOnMatchingLine) // NOLINT
{
    std::vector<std::string> lines{
        "Error executing scheduled query arp_cache: no such table: arp_cache_s",
        "updateSource failed to parse config, of source: /opt/sophos-spl/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.mtr.conf"
    };

    for (const auto& line : lines)
    {
        SCOPED_TRACE(line);

        bool actualResult = OsqueryLogStringUtil::isGenericLogLine(line);

        EXPECT_TRUE(actualResult);
    }
}

TEST_F(TestOsqueryLogStringUtil, testIsGenericLogLineReturnsFalseOnNonMatchingLines) // NOLINT
{
    std::string line = "Some Random Text";

    bool actualResult = OsqueryLogStringUtil::isGenericLogLine(line);

    EXPECT_FALSE(actualResult);

}
