/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gtest/gtest.h>
#include <modules/livequery/Logger.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>

class TestQueryProcessor : public ::testing::Test
{
};

TEST(TestQueryProcessor, TestAQueryCanBePassedTo) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    LOGINFO("Produce this logging");

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Produce this logging"));
}

