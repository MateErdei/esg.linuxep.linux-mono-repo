// Copyright 2020-2022 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/sophos_filesystem.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <gtest/gtest.h>

#include <fstream>

inline sophos_filesystem::path tmpdir()
{
    namespace fs = sophos_filesystem;
    const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    fs::path dir = fs::temp_directory_path();
    dir /= test_info->test_case_name();
    dir /= test_info->name();
    return dir;
}

inline void setupFakeSophosThreatDetectorConfig()
{
    namespace fs = sophos_filesystem;
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

inline void setupFakeSafeStoreConfig()
{
    namespace fs = sophos_filesystem;
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path base = tmpdir();
    appConfig.setData("PLUGIN_INSTALL", base);
    fs::create_directories(base / "var");
}

inline sophos_filesystem::path pluginInstall()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    return appConfig.getData("PLUGIN_INSTALL");
}