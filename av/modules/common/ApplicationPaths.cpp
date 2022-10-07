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

    std::string getSafeStoreFlagPath()
    {
        return getPluginInstall() + "/var/ss_flag.json";
    }

    std::string getSafeStoreDbDirPath()
    {
        return getPluginInstall() + "/var/safestore";
    }

    std::string getSafeStoreDbFileName()
    {
        return "safestore.db";
    }

    std::string getSafeStorePasswordFilePath()
    {
        return getSafeStoreDbDirPath() + "/SafeStore.pw";
    }
} // namespace Plugin