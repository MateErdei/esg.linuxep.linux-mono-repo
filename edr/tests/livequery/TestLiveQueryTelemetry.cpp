/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Helpers/LogInitializedTests.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/livequery/ResponseData.h>
#include <modules/queryrunner/Telemetry.h>
#include <modules/livequery/QueryResponse.h>

#include <thirdparty/nlohmann-json/json.hpp>

#include <gtest/gtest.h>

using namespace ::testing;

queryrunner::QueryRunnerStatus createDummyResponse(
    const std::string& queryName,
    int numberOfRows,
    livequery::ErrorCode errorCode,
    long queryDuration)
{
    queryrunner::QueryRunnerStatus runnerStatus {queryName, errorCode, queryDuration, numberOfRows};
    return runnerStatus;
}

class TestResponseDispatcher : public LogOffInitializedTests{};

TEST_F(TestResponseDispatcher, twoDifferentQueriesShowCorrectTelemetryStats)
{
    auto response1 = createDummyResponse("query1", 2, livequery::ErrorCode::SUCCESS, 10.0);
    auto response2 = createDummyResponse("query2", 3, livequery::ErrorCode::SUCCESS, 20.0);
    auto& telemetryHelper = Common::Telemetry::TelemetryHelper::getInstance();
    queryrunner::Telemetry telemetry;
    telemetry.processLiveQueryResponseStats(response1);
    telemetry.processLiveQueryResponseStats(response2);
    telemetryHelper.updateTelemetryWithStats();

    std::string telemetryJson = telemetryHelper.serialiseAndReset();
    std::cout << telemetryJson;
    auto jsonObject = nlohmann::json::parse(telemetryJson);

    // Query 1
    // Duration
    double q1DurationAvg = jsonObject["live-query"]["query1"]["duration-avg"];
    EXPECT_EQ(q1DurationAvg, 10.0);

    double q1DurationMin = jsonObject["live-query"]["query1"]["duration-min"];
    EXPECT_EQ(q1DurationMin, 10.0);

    double q1DurationMax = jsonObject["live-query"]["query1"]["duration-max"];
    EXPECT_EQ(q1DurationMax, 10.0);

    // Row count
    double q1RowcountAvg = jsonObject["live-query"]["query1"]["rowcount-avg"];
    EXPECT_EQ(q1RowcountAvg, 2.0);

    double q1RowcountMin = jsonObject["live-query"]["query1"]["rowcount-min"];
    EXPECT_EQ(q1RowcountMin, 2.0);

    double q1RowcountMax = jsonObject["live-query"]["query1"]["rowcount-max"];
    EXPECT_EQ(q1RowcountMax, 2.0);

    int q1SuccessfulCount = jsonObject["live-query"]["query1"]["successful-count"];
    EXPECT_EQ(q1SuccessfulCount, 1);

    // Query 2
    // Duration
    double q2DurationAvg = jsonObject["live-query"]["query2"]["duration-avg"];
    EXPECT_EQ(q2DurationAvg, 20.0);

    double q2DurationMin = jsonObject["live-query"]["query2"]["duration-min"];
    EXPECT_EQ(q2DurationMin, 20.0);

    double q2DurationMax = jsonObject["live-query"]["query2"]["duration-max"];
    EXPECT_EQ(q2DurationMax, 20.0);

    // Row count
    double q2RowcountAvg = jsonObject["live-query"]["query2"]["rowcount-avg"];
    EXPECT_EQ(q2RowcountAvg, 3.0);

    double q2RowcountMin = jsonObject["live-query"]["query2"]["rowcount-min"];
    EXPECT_EQ(q2RowcountMin, 3.0);

    double q2RowcountMax = jsonObject["live-query"]["query2"]["rowcount-max"];
    EXPECT_EQ(q2RowcountMax, 3.0);

    int q2SuccessfulCount = jsonObject["live-query"]["query2"]["successful-count"];
    EXPECT_EQ(q2SuccessfulCount, 1);
}

TEST_F(TestResponseDispatcher, twoQueriesSameNameShowCorrectTelemetryStats)
{
    auto response1 = createDummyResponse("query", 100, livequery::ErrorCode::SUCCESS, 800.0);
    auto response2 = createDummyResponse("query", 200, livequery::ErrorCode::SUCCESS, 400.0);
    auto& telemetryHelper = Common::Telemetry::TelemetryHelper::getInstance();
    queryrunner::Telemetry telemetry;
    telemetry.processLiveQueryResponseStats(response1);
    telemetry.processLiveQueryResponseStats(response2);
    telemetryHelper.updateTelemetryWithStats();

    std::string telemetryJson = telemetryHelper.serialiseAndReset();
    std::cout << telemetryJson;
    auto jsonObject = nlohmann::json::parse(telemetryJson);

    double q1DurationAvg = jsonObject["live-query"]["query"]["duration-avg"];
    EXPECT_EQ(q1DurationAvg, 600.0);

    double q1DurationMin = jsonObject["live-query"]["query"]["duration-min"];
    EXPECT_EQ(q1DurationMin, 400.0);

    double q1DurationMax = jsonObject["live-query"]["query"]["duration-max"];
    EXPECT_EQ(q1DurationMax, 800.0);

    double q1RowcountAvg = jsonObject["live-query"]["query"]["rowcount-avg"];
    EXPECT_EQ(q1RowcountAvg, 150.0);

    double q1RowcountMin = jsonObject["live-query"]["query"]["rowcount-min"];
    EXPECT_EQ(q1RowcountMin, 100.0);

    double q1RowcountMax = jsonObject["live-query"]["query"]["rowcount-max"];
    EXPECT_EQ(q1RowcountMax, 200.0);

    int q1SuccessfulCount = jsonObject["live-query"]["query"]["successful-count"];
    EXPECT_EQ(q1SuccessfulCount, 2);
}

TEST_F(TestResponseDispatcher, MultipleQueriesSameNameShowCorrectTelemetryStatsExcludingFailures)
{
    auto response1 = createDummyResponse("query", 100, livequery::ErrorCode::SUCCESS, 800.0);
    auto response2 = createDummyResponse("query", 200, livequery::ErrorCode::SUCCESS, 400.0);
    auto response3 = createDummyResponse("query", 210, livequery::ErrorCode::OSQUERYERROR, 500.0);
    auto response4 = createDummyResponse("query", 220, livequery::ErrorCode::OSQUERYERROR, 600.0);
    auto response5 = createDummyResponse("query", 230, livequery::ErrorCode::OSQUERYERROR, 700.0);
    auto response6 = createDummyResponse("query", 240, livequery::ErrorCode::OSQUERYERROR, 800.0);
    auto& telemetryHelper = Common::Telemetry::TelemetryHelper::getInstance();
    queryrunner::Telemetry telemetry;
    telemetry.processLiveQueryResponseStats(response1);
    telemetry.processLiveQueryResponseStats(response2);
    telemetry.processLiveQueryResponseStats(response3);
    telemetry.processLiveQueryResponseStats(response4);
    telemetry.processLiveQueryResponseStats(response5);
    telemetry.processLiveQueryResponseStats(response6);
    telemetryHelper.updateTelemetryWithStats();

    std::string telemetryJson = telemetryHelper.serialiseAndReset();
    std::cout << telemetryJson;
    auto jsonObject = nlohmann::json::parse(telemetryJson);

    // Query 1
    // Duration
    double q1DurationAvg = jsonObject["live-query"]["query"]["duration-avg"];
    EXPECT_EQ(q1DurationAvg, 600.0);

    double q1DurationMin = jsonObject["live-query"]["query"]["duration-min"];
    EXPECT_EQ(q1DurationMin, 400.0);

    double q1DurationMax = jsonObject["live-query"]["query"]["duration-max"];
    EXPECT_EQ(q1DurationMax, 800.0);

    // Row count
    double q1RowcountAvg = jsonObject["live-query"]["query"]["rowcount-avg"];
    EXPECT_EQ(q1RowcountAvg, 150.0);

    double q1RowcountMin = jsonObject["live-query"]["query"]["rowcount-min"];
    EXPECT_EQ(q1RowcountMin, 100.0);

    double q1RowcountMax = jsonObject["live-query"]["query"]["rowcount-max"];
    EXPECT_EQ(q1RowcountMax, 200.0);

    int q1SuccessfulCount = jsonObject["live-query"]["query"]["successful-count"];
    EXPECT_EQ(q1SuccessfulCount, 2);

    int q1FailedOsqueryErrorCount = jsonObject["live-query"]["query"]["failed-osquery-error-count"];
    EXPECT_EQ(q1FailedOsqueryErrorCount, 4);
}

TEST_F(TestResponseDispatcher, MultipleQueriesSameNameShowCorrectTelemetryStatsExcludingFailures2)
{
    auto response1 = createDummyResponse("query", 100, livequery::ErrorCode::SUCCESS, 800.0);
    auto response2 = createDummyResponse("query", 200, livequery::ErrorCode::SUCCESS, 400.0);
    auto response3 = createDummyResponse("query", 210, livequery::ErrorCode::OSQUERYERROR, 500.0);
    auto response4 = createDummyResponse("query", 220, livequery::ErrorCode::EXTENSIONEXITEDWHILERUNNING, 600.0);
    auto response5 = createDummyResponse("query", 230, livequery::ErrorCode::RESPONSEEXCEEDLIMIT, 700.0);
    auto response6 = createDummyResponse("query", 240, livequery::ErrorCode::UNEXPECTEDERROR, 800.0);
    auto& telemetryHelper = Common::Telemetry::TelemetryHelper::getInstance();
    queryrunner::Telemetry telemetry;
    telemetry.processLiveQueryResponseStats(response1);
    telemetry.processLiveQueryResponseStats(response2);
    telemetry.processLiveQueryResponseStats(response3);
    telemetry.processLiveQueryResponseStats(response4);
    telemetry.processLiveQueryResponseStats(response5);
    telemetry.processLiveQueryResponseStats(response6);
    telemetryHelper.updateTelemetryWithStats();

    std::string telemetryJson = telemetryHelper.serialiseAndReset();
    std::cout << telemetryJson;
    auto jsonObject = nlohmann::json::parse(telemetryJson);

    double q1DurationAvg = jsonObject["live-query"]["query"]["duration-avg"];
    EXPECT_EQ(q1DurationAvg, 600.0);

    double q1DurationMin = jsonObject["live-query"]["query"]["duration-min"];
    EXPECT_EQ(q1DurationMin, 400.0);

    double q1DurationMax = jsonObject["live-query"]["query"]["duration-max"];
    EXPECT_EQ(q1DurationMax, 800.0);

    double q1RowcountAvg = jsonObject["live-query"]["query"]["rowcount-avg"];
    EXPECT_EQ(q1RowcountAvg, 150.0);

    double q1RowcountMin = jsonObject["live-query"]["query"]["rowcount-min"];
    EXPECT_EQ(q1RowcountMin, 100.0);

    double q1RowcountMax = jsonObject["live-query"]["query"]["rowcount-max"];
    EXPECT_EQ(q1RowcountMax, 200.0);

    int q1SuccessfulCount = jsonObject["live-query"]["query"]["successful-count"];
    EXPECT_EQ(q1SuccessfulCount, 2);

    int q1FailedOsqueryErrorCount = jsonObject["live-query"]["query"]["failed-osquery-error-count"];
    EXPECT_EQ(q1FailedOsqueryErrorCount, 1);

    int q1FailedExceedLimitCount = jsonObject["live-query"]["query"]["failed-exceed-limit-count"];
    EXPECT_EQ(q1FailedExceedLimitCount, 1);

    int q1FailedOsqueryDiedCount = jsonObject["live-query"]["query"]["failed-osquery-died-count"];
    EXPECT_EQ(q1FailedOsqueryDiedCount, 1);

    int q1FailedUnexpectedErrorCount = jsonObject["live-query"]["query"]["failed-unexpected-error-count"];
    EXPECT_EQ(q1FailedUnexpectedErrorCount, 1);
}

TEST_F(TestResponseDispatcher, successfulQueriesShowCorrectTelemetryStatsWhenDurationIsZero)
{
    auto response1 = createDummyResponse("query", 2, livequery::ErrorCode::SUCCESS, 0);
    auto response2 = createDummyResponse("query", 3, livequery::ErrorCode::SUCCESS, 0);
    auto& telemetryHelper = Common::Telemetry::TelemetryHelper::getInstance();
    queryrunner::Telemetry telemetry;
    telemetry.processLiveQueryResponseStats(response1);
    telemetry.processLiveQueryResponseStats(response2);
    telemetryHelper.updateTelemetryWithStats();

    std::string telemetryJson = telemetryHelper.serialiseAndReset();
    std::cout << telemetryJson;
    auto jsonObject = nlohmann::json::parse(telemetryJson);

    double q1DurationAvg = jsonObject["live-query"]["query"]["duration-avg"];
    EXPECT_EQ(q1DurationAvg, 0.0);

    double q1DurationMin = jsonObject["live-query"]["query"]["duration-min"];
    EXPECT_EQ(q1DurationMin, 0.0);

    double q1DurationMax = jsonObject["live-query"]["query"]["duration-max"];
    EXPECT_EQ(q1DurationMax, 0.0);

    double q1RowcountAvg = jsonObject["live-query"]["query"]["rowcount-avg"];
    EXPECT_EQ(q1RowcountAvg, 2.5);

    double q1RowcountMin = jsonObject["live-query"]["query"]["rowcount-min"];
    EXPECT_EQ(q1RowcountMin, 2.0);

    double q1RowcountMax = jsonObject["live-query"]["query"]["rowcount-max"];
    EXPECT_EQ(q1RowcountMax, 3.0);

    int q1SuccessfulCount = jsonObject["live-query"]["query"]["successful-count"];
    EXPECT_EQ(q1SuccessfulCount, 2);
}

TEST_F(TestResponseDispatcher, successfulQueriesShowCorrectTelemetryStatsWhenRowCountsAreZero)
{
    auto response1 = createDummyResponse("query", 0, livequery::ErrorCode::SUCCESS, 10);
    auto response2 = createDummyResponse("query", 0, livequery::ErrorCode::SUCCESS, 20);
    auto& telemetryHelper = Common::Telemetry::TelemetryHelper::getInstance();
    queryrunner::Telemetry telemetry;
    telemetry.processLiveQueryResponseStats(response1);
    telemetry.processLiveQueryResponseStats(response2);
    telemetryHelper.updateTelemetryWithStats();

    std::string telemetryJson = telemetryHelper.serialiseAndReset();
    std::cout << telemetryJson;
    auto jsonObject = nlohmann::json::parse(telemetryJson);

    double q1DurationAvg = jsonObject["live-query"]["query"]["duration-avg"];
    EXPECT_EQ(q1DurationAvg, 15.0);

    double q1DurationMin = jsonObject["live-query"]["query"]["duration-min"];
    EXPECT_EQ(q1DurationMin, 10.0);

    double q1DurationMax = jsonObject["live-query"]["query"]["duration-max"];
    EXPECT_EQ(q1DurationMax, 20.0);

    double q1RowcountAvg = jsonObject["live-query"]["query"]["rowcount-avg"];
    EXPECT_EQ(q1RowcountAvg, 0.0);

    double q1RowcountMin = jsonObject["live-query"]["query"]["rowcount-min"];
    EXPECT_EQ(q1RowcountMin, 0.0);

    double q1RowcountMax = jsonObject["live-query"]["query"]["rowcount-max"];
    EXPECT_EQ(q1RowcountMax, 0.0);

    int q1SuccessfulCount = jsonObject["live-query"]["query"]["successful-count"];
    EXPECT_EQ(q1SuccessfulCount, 2);
}