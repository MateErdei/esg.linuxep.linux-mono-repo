/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginUtils.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <fstream>

namespace fs = sophos_filesystem;

namespace common
{
    std::string getPluginVersion()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path versionFile(appConfig.getData("PLUGIN_INSTALL"));
        versionFile /= "VERSION.ini";

        std::string versionKeyword("PRODUCT_VERSION = ");
        std::ifstream versionFileFs(versionFile);
        if (versionFileFs.good())
        {
            std::string line;
            while (std::getline(versionFileFs, line))
            {
                if (line.rfind(versionKeyword, 0) == 0)
                {
                    return line.substr(versionKeyword.size(), line.size());
                }
            }
        }
        return "unknown";
    }

    fs::path getPluginInstallPath()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        return appConfig.getData("PLUGIN_INSTALL");
    }
}