// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "common/config.h"
#include "datatypes/sophos_filesystem.h"
#include "sophos_on_access_process/soapd_bootstrap/SoapdBootstrap.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Logging/PluginLoggingSetup.h"
#include "Common/Main/Main.h"

namespace fs = sophos_filesystem;

static int inner_main()
{
    Common::Logging::PluginLoggingSetup setupFileLoggingWithPath(PLUGIN_NAME, "soapd");
    // PLUGIN_INSTALL
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    fs::path pluginInstall = sophosInstall / "plugins" / PLUGIN_NAME;
    appConfig.setData("PLUGIN_INSTALL", pluginInstall);

    return sophos_on_access_process::soapd_bootstrap::SoapdBootstrap::runSoapd();
}
MAIN(inner_main())
