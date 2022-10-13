// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ApplicationPaths.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <sstream>
#include <stdexcept>

namespace Plugin
{
    std::string getPluginInstall()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        try
        {
            return appConfig.getData("PLUGIN_INSTALL");
        }
        catch (const std::exception& ex)
        {
            std::stringstream errorMessage;
            errorMessage << "Failed to read PLUGIN_INSTALL from app config: " << ex.what();
            throw std::runtime_error(errorMessage.str());
        }
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