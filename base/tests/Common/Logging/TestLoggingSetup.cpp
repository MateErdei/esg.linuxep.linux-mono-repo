/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/Logging/PluginLoggingSetup.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(TestLoggingSetup, TestConsoleSetup) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup setup;
}

TEST(TestLoggingSetup, TestFileSetup) // NOLINT
{
    Common::Logging::FileLoggingSetup setup("testlogging");
}

TEST(TestLoggingSetup, TestPluginSetup) // NOLINT
{
    Common::Logging::PluginLoggingSetup setup("PluginName");
}
