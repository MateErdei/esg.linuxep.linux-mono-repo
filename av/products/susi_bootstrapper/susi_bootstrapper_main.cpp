/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "common/config.h"
#include "datatypes/sophos_filesystem.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Susi.h>

namespace fs = sophos_filesystem;

const char* PluginName = PLUGIN_NAME;

int main()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    fs::path pluginInstall = sophosInstall / "plugins" / PluginName;
    fs::path updateSource = pluginInstall / "chroot/susi/update_source";
    fs::path installDest = pluginInstall / "chroot/susi/distribution_version";

    std::cout << "Bootstrapping SUSI from update source: " << updateSource << std::endl;
    SusiResult susiResult = SUSI_Install(updateSource.c_str(), installDest.c_str());
    if (susiResult == SUSI_S_OK)
    {
        std::cout << "Successfully installed SUSI to: "  << installDest << std::endl;
    }
    else
    {
        std::cerr << "Failed to bootstrap SUSI with error: " << susiResult << std::endl;
    }
    return susiResult;
}