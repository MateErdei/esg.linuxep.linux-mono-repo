// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include <gtest/gtest.h>

#include "pluginimpl/Logger.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"

using namespace Common::FileSystem;

TEST(TestFileSystemExample, binBashShouldExist) // NOLINT
{
    auto* ifileSystem = Common::FileSystem::fileSystem();
    EXPECT_TRUE(ifileSystem->isExecutable("/bin/bash"));
}

TEST(TestLinkerWorks, WorksWithTheLogging) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    LOGINFO("Produce this logging");
}
