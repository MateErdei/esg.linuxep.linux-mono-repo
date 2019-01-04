/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <Common/FileSystem/IFileSystem.h>
#include <modules/pluginimpl/Logger.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include "gtest/gtest.h"

using namespace Common::FileSystem;

TEST(TestFileSystemExample, binBashShouldExist) // NOLINT
{
    auto * ifileSystem = Common::FileSystem::fileSystem();
    EXPECT_TRUE(ifileSystem->isExecutable("/bin/bash"));
}


TEST(TestLinkerWorks, WorksWithTheLogging) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    LOGINFO("Produce this logging");
}