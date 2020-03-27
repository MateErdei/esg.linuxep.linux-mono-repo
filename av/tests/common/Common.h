/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "datatypes/sophos_filesystem.h"
#include <fstream>

#define BASE "/tmp/TestPluginAdapter"
namespace fs = sophos_filesystem;

void setupFakeSophosThreatDetectorConfig()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", BASE);
    fs::path f = BASE;
    fs::create_directories(f / "chroot");
    f /= "sbin";
    fs::create_directories(f);
    f /= "sophos_threat_detector_launcher";
    std::ofstream ost(f);
    ost.close();
}