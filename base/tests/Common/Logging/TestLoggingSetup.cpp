/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "modules/Common/Logging/ConsoleLoggingSetup.h"
#include "modules/Common/Logging/FileLoggingSetup.h"
#include "modules/Common/Logging/PluginLoggingSetup.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(TestLoggingSetup, TestConsoleSetup) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup setup;
}

TEST(TestLoggingSetup, TestFileSetup) // NOLINT
{
    Common::Logging::FileLoggingSetup setup("testlogging", false);
}

TEST(TestLoggingSetup, TestPluginSetup) // NOLINT
{
    Common::Logging::PluginLoggingSetup setup("PluginName");
}
