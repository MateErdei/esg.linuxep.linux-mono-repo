// Copyright 2023 Sophos Limited. All rights reserved.

#include "LogSetup.h"

#include "common/config.h"
#include "datatypes/sophos_filesystem.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/SophosThreatDetectorMain.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <sys/stat.h>

namespace fs = sophos_filesystem;

static const char* PluginName = PLUGIN_NAME;

int inner_main()
{
    // PLUGIN_INSTALL
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    fs::path pluginInstall = sophosInstall / "plugins" / PluginName;
    appConfig.setData("PLUGIN_INSTALL", pluginInstall);

    LogSetup logging;
    //Only allow the owner to read/write/execute, SUSI's least permissive umask is 077
    ::umask(077);
    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    return treatDetectorMain.sophos_threat_detector_main();
}
