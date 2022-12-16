// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/sophos_filesystem.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Helpers/TempDir.h"

#include <gtest/gtest.h>

#include <fstream>

inline std::unique_ptr<Tests::TempDir> setupFakePluginDir()
{
    const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    auto tempDir = Tests::TempDir::makeTempDir(test_info->test_case_name());
    appConfig.setData("PLUGIN_INSTALL", tempDir->dirPath());
    tempDir->makeDirs("var");
    tempDir->makeDirs("chroot/var/sbin");
    tempDir->createFile("chroot/var/sbin/sophos_threat_detector_launcher", "");
    return tempDir;
}
