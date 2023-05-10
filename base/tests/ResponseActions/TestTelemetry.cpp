// Copyright 2023 Sophos Limited. All rights reserved.

#include "ResponseActions/RACommon/ResponseActionsCommon.h"
#include "ResponseActions/ResponsePlugin/Telemetry.h"
#include "ResponseActions/ResponsePlugin/TelemetryConsts.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include "tests/Common/Helpers/MemoryAppender.h"

#include <gtest/gtest.h>

using namespace ResponsePlugin::Telemetry;
using namespace ResponseActions::RACommon;

const std::vector<std::string> RUNCOMMAND_TELEMETRY_FIELDS
    {RUN_COMMAND_COUNT, RUN_COMMAND_FAILED_COUNT, RUN_COMMAND_TIMEOUT_COUNT, RUN_COMMAND_EXPIRED_COUNT };

const std::vector<std::string> UPLOADFILE_TELEMETRY_FIELDS
    {UPLOAD_FILE_COUNT, UPLOAD_FILE_FAILED_COUNT, UPLOAD_FILE_TIMEOUT_COUNT, UPLOAD_FILE_EXPIRED_COUNT };

const std::vector<std::string> UPLOADFOLDER_TELEMETRY_FIELDS
    {UPLOAD_FOLDER_COUNT, UPLOAD_FOLDER_FAILED_COUNT, UPLOAD_FOLDER_TIMEOUT_COUNT, UPLOAD_FOLDER_EXPIRED_COUNT };

const std::vector<std::string> DOWNLOAD_TELEMETRY_FIELDS
    {DOWNLOAD_FILE_COUNT, DOWNLOAD_FILE_FAILED_COUNT, DOWNLOAD_FILE_TIMEOUT_COUNT, DOWNLOAD_FILE_EXPIRED_COUNT };

class TestTelemetry  : public MemoryAppenderUsingTests
{
public:
    TestTelemetry()
        : MemoryAppenderUsingTests("responseactions")
    {}
    void SetUp()
    {}
};

TEST_F(TestTelemetry, IncrementTotalActionsThrowsOnUnknownRequestType)
{
    try
    {
        incrementTotalActions("notatype");
        FAIL() << "Invalid type ignored";
    }
    catch (const std::logic_error& e)
    {
        EXPECT_STREQ(e.what(), "Unknown action type provided to incrementTotalActions: notatype");
    }
}

TEST_F(TestTelemetry, IncrementTimedOutActionsThrowsOnUnknownRequestType)
{
    try
    {
        incrementTimedOutActions("notatype");
        FAIL() << "Invalid type ignored";
    }
    catch (const std::logic_error& e)
    {
        EXPECT_STREQ(e.what(), "Unknown action type provided to incrementTimedOutActions: notatype");
    }
}

TEST_F(TestTelemetry, IncrementFailedActionsThrowsOnUnknownRequestType)
{
    try
    {
        incrementFailedActions("notatype");
        FAIL() << "Invalid type ignored";
    }
    catch (const std::logic_error& e)
    {
        EXPECT_STREQ(e.what(), "Unknown action type provided to incrementFailedActions: notatype");
    }
}

TEST_F(TestTelemetry, IncrementExpiredActionsThrowsOnUnknownRequestType)
{
    try
    {
        incrementExpiredActions("notatype");
        FAIL() << "Invalid type ignored";
    }
    catch (const std::logic_error& e)
    {
        EXPECT_STREQ(e.what(), "Unknown action type provided to incrementExpiredActions: notatype");
    }
}

class TestTelemetryParameterized
    : public ::testing::TestWithParam<std::pair<std::string, std::vector<std::string>>>
{
protected:
    void SetUp() override
    {
        //Clear telemetry before a test
        std::ignore = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
        m_loggingSetup = Common::Logging::LOGOFFFORTEST();
    }
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

INSTANTIATE_TEST_SUITE_P(
    TestTelemetry,
    TestTelemetryParameterized,
    ::testing::Values(
        std::make_pair(RUN_COMMAND_REQUEST_TYPE, RUNCOMMAND_TELEMETRY_FIELDS),
        std::make_pair(UPLOAD_FILE_REQUEST_TYPE, UPLOADFILE_TELEMETRY_FIELDS),
        std::make_pair(UPLOAD_FOLDER_REQUEST_TYPE, UPLOADFOLDER_TELEMETRY_FIELDS),
        std::make_pair(DOWNLOAD_FILE_REQUEST_TYPE, DOWNLOAD_TELEMETRY_FIELDS)
            ));


TEST_P(TestTelemetryParameterized, ValidTelemetryTypes)
{
    auto [request, telemetryfields] = GetParam();

    incrementTotalActions(request);
    auto telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry, R"({")" + telemetryfields.at(0) + R"(":1})");

    incrementFailedActions(request);
    telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry, R"({")" + telemetryfields.at(1) + R"(":1})");

    incrementTimedOutActions(request);
    telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry, R"({")" + telemetryfields.at(2) + R"(":1})");

    incrementExpiredActions(request);
    telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry, R"({")" + telemetryfields.at(3) + R"(":1})");
}