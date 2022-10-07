// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ApplicationPaths.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

namespace Plugin
{
    std::string getPluginInstall()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        return appConfig.getData("PLUGIN_INSTALL");
    }

    std::string getSafeStorePidPath()
    {
        return getPluginInstall() +  "/var/safestore.pid";
    }

    std::string getSafeStoreSocketPath()
    {
        return getPluginInstall() + "/var/safestore_socket";
    }
} // namespace Plugin