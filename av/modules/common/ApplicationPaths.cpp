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

    std::string getPluginVarDirPath()
    {
        return getPluginInstall() + "/var";
    }

    std::string getSafeStorePidPath()
    {
        return getPluginVarDirPath() + "/safestore.pid";
    }

    std::string getSafeStoreSocketPath()
    {
        return getPluginVarDirPath() + "/safestore_socket";
    }

    std::string getSafeStoreDbDirPath()
    {
        return getPluginVarDirPath() + "/safestore_db";
    }

    std::string getSafeStoreDbFileName()
    {
        return "safestore.db";
    }

    std::string getSafeStorePasswordFilePath()
    {
        return getSafeStoreDbDirPath() + "/safestore.pw";
    }

    std::string getSafeStoreConfigPath()
    {
        return getPluginVarDirPath() + "/safestore_config.json";
    }
} // namespace Plugin