// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include <Common/Helpers/LogInitializedTests.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/livequery/ResponseData.h>
#include <modules/queryrunner/Telemetry.h>
#include <modules/livequery/QueryResponse.h>

#include <nlohmann/json.hpp>

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

namespace
{
    class TestLiveQueryTelemetry : public LogOffInitializedTests
    {
    };
}

TEST_F(TestLiveQueryTelemetry, MultipleQueriesSameNameShowCorrectTelemetryStatsExcludingFailures2)
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
    auto jsonObject = nlohmann::json::parse(telemetryJson);

    int q1SuccessfulCount = jsonObject["live-query"]["successful-count"];
    EXPECT_EQ(q1SuccessfulCount, 2);

    int q1FailedOsqueryErrorCount = jsonObject["live-query"]["failed-count"];
    EXPECT_EQ(q1FailedOsqueryErrorCount, 4);


}
