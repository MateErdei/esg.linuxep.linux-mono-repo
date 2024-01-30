// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "pluginimpl/OsqueryLogStringUtil.h"

#ifdef SPL_BAZEL
#include "tests/Common/Helpers/LogInitializedTests.h"
#else
#include "Common/Helpers/LogInitializedTests.h"
#endif

#include <gtest/gtest.h>

class TestOsqueryLogStringUtil : public LogOffInitializedTests{};

TEST_F(TestOsqueryLogStringUtil, testProcessOsqueryLogLineForScheduledQueriesReturnsCorrectFormattedString)
{
    std::vector<std::string> lines{
        "I0215 15:47:24.150650  8842 scheduler.cpp:102] Executing scheduled query host_sensor_heartbeat_check: SELECT",
        "    \"deI0310 16:50:56.705953 35257 scheduler.cpp:103] Executing scheduled query running_processes_linux_events: SELECT"   // due to invalid JSON in config file
    };
    std::vector<std::string> expected{
        "Executing query: host_sensor_heartbeat_check",
        "Executing query: running_processes_linux_events"
    };

    for (size_t index=0 ; index<lines.size() ; index++)
    {
        std::optional<std::string> actualResult = OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(lines[index]);

        ASSERT_TRUE(actualResult.has_value());
        EXPECT_EQ(actualResult.value(), expected[index]);
    }
}

TEST_F(TestOsqueryLogStringUtil, testProcessOsqueryLogLineForScheduledQueriesReturnsFalseForUninterestingStrings)
{
    std::string line = "I0215 15:47:24.150650  8842 scheduler.cpp:102] hello: SELECT";

    std::optional<std::string> actualResult = OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(line);

    EXPECT_EQ(actualResult.has_value(), false);
}

TEST_F(TestOsqueryLogStringUtil, testProcessOsqueryLogLineForScheduledQueriesReturnsFalseForFlushqueries)
{
    std::string line = "I0215 15:47:24.150650  8842 scheduler.cpp:102] Executing scheduled query flush_process_events: SELECT";

    std::optional<std::string> actualResult = OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(line);

    EXPECT_EQ(actualResult.has_value(), false);
}

TEST_F(TestOsqueryLogStringUtil, testProcessOsqueryLogLineForScheduledQueriesReturnsFalseOnMalformedLine)
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

TEST_F(TestOsqueryLogStringUtil, testIsGenericLogLineReturnsTrueOnMatchingLine)
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

TEST_F(TestOsqueryLogStringUtil, testIsGenericLogLineReturnsFalseOnNonMatchingLines)
{
    std::string line = "Some Random Text";

    bool actualResult = OsqueryLogStringUtil::isGenericLogLine(line);

    EXPECT_FALSE(actualResult);

}

TEST_F(TestOsqueryLogStringUtil, testProcessOsqueryLogLineForScheduledQueriesHandlesMissingQueryName)
{
    std::vector<std::string> lines{
        "I0215 15:47:24.150650  8842 scheduler.cpp:102] Executing scheduled query ",
        "I0310 16:50:56.705953 35257 scheduler.cpp:103] Executing scheduled query",
        "I0310 16:50:56.705953 35257 scheduler.cpp:103] Executing scheduled query :",
        "I03sss10 16:5asdasdasdddd705sdsd57 scheduler.cpp:103] Executing scheduled query : a query",
    };
    for (auto& line : lines)
    {
        std::optional<std::string> actualResult = OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(line);
        ASSERT_FALSE(actualResult) << " Failed because this value was set: " + actualResult.value();
    }
}

TEST_F(TestOsqueryLogStringUtil, testProcessOsqueryLogLineForScheduledQueriesHandlesLongQueryName)
{
    std::stringstream line;
    line << "I0215 15:47:24.150650  8842 scheduler.cpp:102] Executing scheduled query ";
    std::stringstream expected;
    expected <<  "Executing query: ";
    for (int i = 0; i < 1000000; ++i)
    {
        line << "a";
        expected << "a";
    }
    std::optional<std::string> actualResult = OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(line.str());
    ASSERT_TRUE(actualResult);
    ASSERT_EQ(actualResult.value(), expected.str());
}

TEST_F(TestOsqueryLogStringUtil, testProcessOsqueryLogLineForScheduledQueriesHandlesNonAsciiChars)
{
    std::string line =  "I0215 15:47:24.150650  8842 scheduler.cpp:102] Executing scheduled query query_name_平仮名_片仮名: query ";
    std::string expected = "Executing query: query_name_平仮名_片仮名";
    std::optional<std::string> actualResult = OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(line);
    ASSERT_TRUE(actualResult);
    ASSERT_EQ(actualResult.value(), expected);
}