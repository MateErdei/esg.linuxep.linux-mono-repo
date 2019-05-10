/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "tests/Telemetry/MockSystemTelemetryCollector.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Telemetry/TelemetryImpl/SystemTelemetryReporter.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/Common/Process/IProcess.h>

#include <map>
#include <regex>
#include <utility>
#include <vector>

using ::testing::Return;
class SystemTelemetryReporterTests : public ::testing::Test
{
public:
    void TearDown() override {}

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(SystemTelemetryReporterTests, getTelemetryEmptyOK) // NOLINT
{
    std::map<std::string, Telemetry::TelemetryItem> emptySimpleObjs;
    std::map<std::string, std::vector<Telemetry::TelemetryItem>> emptyArrayObjs;

    MockSystemTelemetryCollector mockCollector;
    Telemetry::SystemTelemetryReporter reporter(mockCollector);

    EXPECT_CALL(mockCollector, collectObjects()).WillOnce(Return(emptySimpleObjs));
    EXPECT_CALL(mockCollector, collectArraysOfObjects()).WillOnce(Return(emptyArrayObjs));

    auto sysTelemetryJSON = reporter.getTelemetry();

    ASSERT_EQ(sysTelemetryJSON, "{}");
}

TEST_F(SystemTelemetryReporterTests, getTelemetryOk) // NOLINT
{
    std::map<std::string, Telemetry::TelemetryItem> simpleObjs = {
        { "test-simple-string", { { "", { "test-version 10.5 string" } } } },
        { "test-simple-int", { { "", { 101 } } } }
    };

    std::map<std::string, std::vector<Telemetry::TelemetryItem>> arrayObjs = {
        { "test-array",
          { { { "fstype", { "testvalue1" } }, { "free", { 201 } } },
            { { "fstype", { "testvalue2" } }, { "free", { 202 } } } } }
    };

    MockSystemTelemetryCollector mockCollector;
    Telemetry::SystemTelemetryReporter reporter(mockCollector);

    EXPECT_CALL(mockCollector, collectObjects()).WillOnce(Return(simpleObjs));
    EXPECT_CALL(mockCollector, collectArraysOfObjects()).WillOnce(Return(arrayObjs));

    auto sysTelemetryJSON = reporter.getTelemetry();

    ASSERT_TRUE(sysTelemetryJSON.find("test-simple-string") != std::string::npos);
    ASSERT_TRUE(sysTelemetryJSON.find("test-simple-int") != std::string::npos);
    ASSERT_TRUE(sysTelemetryJSON.find("test-array") != std::string::npos);
}

TEST_F(SystemTelemetryReporterTests, getTelemetryThrowsButNoTopLevelExpectedException) // NOLINT
{
    std::map<std::string, Telemetry::TelemetryItem> simpleObjs = { { "test-unexpected-object",
                                                                     { { "unexpected-name", { 42 } } } } };

    MockSystemTelemetryCollector mockCollector;
    Telemetry::SystemTelemetryReporter reporter(mockCollector);

    EXPECT_CALL(mockCollector, collectObjects()).WillOnce(Return(simpleObjs));

    ASSERT_ANY_THROW(reporter.getTelemetry());
}
