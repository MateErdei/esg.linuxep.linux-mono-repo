// Copyright 2018-2024 Sophos Limited. All rights reserved.
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Logging/FileLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"
#include "Common/Logging/PluginLoggingSetup.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

TEST(TestLoggingSetup, TestConsoleSetup)
{
    Common::Logging::ConsoleLoggingSetup setup;
}

TEST(TestLoggingSetup, TestFileSetup)
{
    Common::Logging::FileLoggingSetup setup("testlogging", false);
}

TEST(TestLoggingSetup, FileLoggingSetupDoesNotPrintErrorsToStderr)
{
    Common::Logging::FileLoggingSetup setup{ "testlogging", false };
    const auto logger = Common::Logging::getInstance("logger");

    ::testing::internal::CaptureStderr();
    LOG4CPLUS_ERROR(logger, "Error message");
    const auto capturedStderr = ::testing::internal::GetCapturedStderr();

    EXPECT_EQ(capturedStderr, "");
}

TEST(TestLoggingSetup, FileLoggingSetupDoesPrintsFatalsToStderr)
{
    Common::Logging::FileLoggingSetup setup{ "testlogging", false };
    const auto logger = Common::Logging::getInstance("logger");

    ::testing::internal::CaptureStderr();
    LOG4CPLUS_FATAL(logger, "Fatal message");
    const auto capturedStderr = ::testing::internal::GetCapturedStderr();

    EXPECT_THAT(capturedStderr, ::testing::HasSubstr("FATAL Fatal message"));
}

TEST(TestLoggingSetup, TestPluginSetup)
{
    Common::Logging::PluginLoggingSetup setup("PluginName");
}
