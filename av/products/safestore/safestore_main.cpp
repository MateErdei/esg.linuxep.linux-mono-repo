// Copyright 2022, Sophos Limited.  All rights reserved.

#include "../common/config.h"
#include "datatypes/sophos_filesystem.h"
#include "safestore/Main.h"

#include "Common/Logging/PluginLoggingSetup.h"
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <sys/stat.h>

namespace fs = sophos_filesystem;

int main()
{
    umask(S_IRWXG | S_IRWXO | S_IXUSR); // Read and write for the owner
    Common::Logging::PluginLoggingSetup setupFileLoggingWithPath(PLUGIN_NAME, "safestore");
    // PLUGIN_INSTALL
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    fs::path pluginInstall = sophosInstall / "plugins" / PLUGIN_NAME;
    appConfig.setData("PLUGIN_INSTALL", pluginInstall);

    return safestore::Main::run();
}
