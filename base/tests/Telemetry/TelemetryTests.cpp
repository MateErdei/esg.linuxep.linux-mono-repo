/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Executable
 */

#include <Telemetry/Telemetry.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class TelemetryTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        Test::SetUp();
    }

    void TearDown() override
    {
        Test::TearDown();
    }
};

TEST_F(TelemetryTest, main_entry_ReturnsSuccess) // NOLINT
{
    std::vector<std::string> arguments = {"arg1", "arg2"};

    std::vector<char*> argv;
    for (const auto& arg : arguments)
        argv.push_back((char*)arg.data());
    argv.push_back(nullptr);

    int expectedErrorCode = 0;

    EXPECT_EQ(Telemetry::main_entry(argv.size() - 1, argv.data()), expectedErrorCode);
}