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

    std::string getPluginChrootDirPath()
    {
        return getPluginInstall() + "/chroot";
    }

    std::string getPluginChrootVarDirPath()
    {
        return getPluginChrootDirPath() + "/var";
    }

    std::string getSafeStorePidPath()
    {
        return getPluginVarDirPath() + "/safestore.pid";
    }

    std::string getSafeStoreSocketPath()
    {
        return getPluginVarDirPath() + "/safestore_socket";
    }

    std::string getSafeStoreRescanSocketPath()
    {
        return getPluginChrootVarDirPath() + "/safestore_rescan_socket";
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

    std::string getSafeStoreDormantFlagPath()
    {
        return getPluginVarDirPath() + "/safestore_dormant_flag";
    }

    std::string getSoapdPidPath()
    {
        return getPluginVarDirPath() + "/soapd.pid";
    }

    std::string getOnAccessUnhealthyFlagPath()
    {
        return getPluginVarDirPath() + "/onaccess_unhealthy_flag";
    }

    std::string getRelativeSafeStoreRescanIntervalConfigPath()
    {
        return "/var/safeStoreRescanInterval";
    }
} // namespace Plugin