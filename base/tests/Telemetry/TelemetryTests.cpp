/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Executable
 */

#include <Telemetry/Telemetry.h>
#include <gtest/gtest.h>

class TelemetryTest : public ::testing::Test
{
};

TEST(TelemetryTest, main_entry_GetRequestReturnsSuccess) // NOLINT
{
    std::vector<std::string> arguments = {"/opt/sophos-spl/base/bin/telemetry", "localhost", "4443", "GET"};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    EXPECT_EQ(Telemetry::main_entry(argv.size(), argv.data()), expectedErrorCode);
}

TEST(TelemetryTest, main_entry_PostRequestReturnsSuccess) // NOLINT
{
    std::vector<std::string> arguments = {"/opt/sophos-spl/base/bin/telemetry", "localhost", "4443", "POST"};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    EXPECT_EQ(Telemetry::main_entry(argv.size(), argv.data()), expectedErrorCode);
}