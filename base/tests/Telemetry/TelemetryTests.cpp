/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Executable
 */

#include <Telemetry/TelemetryImpl/Telemetry.h>

#include <gtest/gtest.h>

class TelemetryTest : public ::testing::Test
{
};

TEST(TelemetryTest, main_entry_ReturnsSuccess) // NOLINT
{
    std::vector<std::string> arguments = {"arg1", "arg2"};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
    {
        argv.emplace_back(const_cast<char*>(arg.data()));
    }

    int expectedErrorCode = 0;

    EXPECT_EQ(Telemetry::main_entry(argv.size(), argv.data()), expectedErrorCode);
}