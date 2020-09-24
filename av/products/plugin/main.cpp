/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "../common/config.h"

#include "modules/datatypes/sophos_filesystem.h"

#include <Common/Logging/PluginLoggingSetup.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/ErrorCodes.h>

#include <pluginimpl/Logger.h>
#include <pluginimpl/PluginAdapter.h>

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

const char* PluginName = PLUGIN_NAME;

namespace fs = sophos_filesystem;

int main()
{
    // PLUGIN_INSTALL
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    fs::path pluginInstall = sophosInstall / "plugins" / PluginName;
    appConfig.setData("PLUGIN_INSTALL", pluginInstall);

    using namespace Plugin;
    int ret = 0;
    Common::Logging::PluginLoggingSetup loggerSetup(PluginName);

    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement =
        Common::PluginApi::createPluginResourceManagement();

    auto queueTask = std::make_shared<QueueTask>();
    auto sharedPluginCallBack = std::make_shared<PluginCallback>(queueTask);

    std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;

    try
    {
        baseService = resourceManagement->createPluginAPI(PluginName, sharedPluginCallBack);
    }
    catch (const Common::PluginApi::ApiException & apiException)
    {
        LOGERROR("Failed to instantiate Plugin Api: " << apiException.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), sharedPluginCallBack);

    // If this is the first time restoring this will also set the telemetry backup file name
    Common::Telemetry::TelemetryHelper::getInstance().restore("av");

    try
    {
        pluginAdapter.mainLoop();
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Exception caught from plugin at top level: " << ex.what());
        ret = 40;
    }

    Common::Telemetry::TelemetryHelper::getInstance().save();

    LOGINFO("Exiting AV plugin");
    return ret;
}
