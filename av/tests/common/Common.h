/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "datatypes/sophos_filesystem.h"
#include <fstream>

#define BASE "/tmp/TestPluginAdapter"
namespace fs = sophos_filesystem;

const fs::path tmpdir()
{
    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    fs::path dir = fs::temp_directory_path();
    dir /= test_info->test_case_name();
    dir /= test_info->name();
    return dir;
}

void setupFakeSophosThreatDetectorConfig()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path base = tmpdir();
    appConfig.setData("PLUGIN_INSTALL", base);
    fs::path f = base;
    fs::create_directories(f / "chroot/var");
    f /= "sbin";
    fs::create_directories(f);
    f /= "sophos_threat_detector_launcher";
    std::ofstream ost(f);
    ost.close();
}

fs::path pluginInstall()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    return appConfig.getData("PLUGIN_INSTALL");
}