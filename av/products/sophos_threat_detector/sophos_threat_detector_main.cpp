/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LogSetup.h"

#include "../common/config.h"
#include "datatypes/sophos_filesystem.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/SophosThreatDetectorMain.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

namespace fs = sophos_filesystem;

const char* PluginName = PLUGIN_NAME;

int main()
{
    // PLUGIN_INSTALL
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    fs::path pluginInstall = sophosInstall / "plugins" / PluginName;
    appConfig.setData("PLUGIN_INSTALL", pluginInstall);

    LogSetup logging;
    return sspl::sophosthreatdetectorimpl::sophos_threat_detector_main();
}
