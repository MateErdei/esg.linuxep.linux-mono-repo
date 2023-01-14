// Copyright 2022, Sophos Limited.  All rights reserved.

#include "../common/config.h"
#include "datatypes/sophos_filesystem.h"
#include "sophos_on_access_process/soapd_bootstrap/SoapdBootstrap.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Logging/PluginLoggingSetup.h"

namespace fs = sophos_filesystem;

int main()
{
    Common::Logging::PluginLoggingSetup setupFileLoggingWithPath(PLUGIN_NAME, "soapd");
    // PLUGIN_INSTALL
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    fs::path pluginInstall = sophosInstall / "plugins" / PLUGIN_NAME;
    appConfig.setData("PLUGIN_INSTALL", pluginInstall);

    return sophos_on_access_process::soapd_bootstrap::SoapdBootstrap::runSoapd();
}
