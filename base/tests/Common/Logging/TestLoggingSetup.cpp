/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Logging/FileLoggingSetup.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

TEST(TestLoggingSetup, TestConsoleSetup) //NOLINT
{
    Common::Logging::ConsoleLoggingSetup setup;
}

TEST(TestLoggingSetup, TestFileSetup) //NOLINT
{
    Common::Logging::FileLoggingSetup setup("testlogging");
}
