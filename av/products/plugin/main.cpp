/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "config.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/PluginLoggingSetup.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/PluginApi/ApiException.h>
#include <modules/pluginimpl/Logger.h>
#include <modules/pluginimpl/PluginAdapter.h>

const char* PluginName = PLUGIN_NAME;

int main()
{
    using namespace Plugin;
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
        LOGERROR("Unexpected error: " << apiException.what());
        throw;
    }

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), sharedPluginCallBack);

    try
    {
        pluginAdapter.mainLoop();
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Plugin threw an exception at top level: " << ex.what());
    }
    LOGINFO("Plugin Finished.");
}
