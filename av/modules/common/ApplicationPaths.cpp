// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "ApplicationPaths.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <sstream>
#include <stdexcept>

namespace Plugin
{
    std::string getMetadataRescanSocketPath()
    {
        return getPluginChrootVarDirPath() + "/metadata_rescan_socket";
    }

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

    std::string getRestoreReportSocketPath()
    {
        return getPluginVarDirPath() + "/restore_report_socket";
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

    std::string getSafeStoreDbLockDirPath()
    {
        return getSafeStoreDbDirPath() + "/safestore.db.lock";
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

    std::string getThreatDetectorUnhealthyFlagPath()
    {
        return getPluginChrootVarDirPath() + "/threatdetector_unhealthy_flag";
    }

    std::string getThreatDetectorSusiUpdateStatusPath()
    {
        return getPluginChrootVarDirPath() + "/update_status.json";
    }

    std::string getRelativeSafeStoreRescanIntervalConfigPath()
    {
        return "/var/safeStoreRescanInterval";
    }

    std::string getSafeStoreConfigPath()
    {
        return getPluginVarDirPath() + "/safestore_config.json";
    }

    std::string getScanningSocketPath()
    {
        return getPluginChrootVarDirPath() + "/scanning_socket";
    }

    std::string getOnAccessStatusPath()
    {
        return getPluginVarDirPath() + "/on_access_status";
    }

    std::string getDisableSafestorePath()
    {
        return getPluginVarDirPath() + "/disable_safestore";
    }

    std::string getPersistThreatDatabaseFilePath()
    {
        return getPluginVarDirPath() + "/persist-threatDatabase";
    }

    std::string getSusiStartupSettingsPath()
    {
        auto pluginInstall = getPluginInstall();
        return pluginInstall + "/var/susi_startup_settings.json";
    }
} // namespace Plugin