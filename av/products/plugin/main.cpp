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
        LOGERROR("Instantiating    Plugin Api failed: " << apiException.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), sharedPluginCallBack);

    try
    {
        pluginAdapter.mainLoop();
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Caught    exception that plugin threw at top level: " << ex.what());
        ret = 40;
    }
    // TODO: Should this be LOGSUPPORT?
    LOGINFO("Exiting    plugin");
    return ret;
}
