/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "config.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/PluginLoggingSetup.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/ErrorCodes.h>
#include <modules/pluginimpl/Logger.h>
#include <modules/pluginimpl/PluginAdapter.h>
#include <modules/livequery/ResponseDispatcher.h>
#include <modules/osqueryclient/OsqueryProcessor.h>


const char* PluginName = PLUGIN_NAME;

int main()
{
    using namespace Plugin;
    int ret = 0;
    Common::Logging::PluginLoggingSetup loggerSetup(PluginName);
    std::string socket = "/tmp/test.";
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
        LOGERROR("Plugin Api could not be instantiated: " << apiException.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }
    std::unique_ptr<livequery::IQueryProcessor> queryProcessor(new osqueryclient::OsqueryProcessor{Plugin::osquerySocket()});
    std::unique_ptr<livequery::IResponseDispatcher> queryResponder(new livequery::ResponseDispatcher{});



    PluginAdapter pluginAdapter(queueTask, std::move(baseService), sharedPluginCallBack,
                                std::move(queryProcessor), std::move(queryResponder));

    try
    {
        pluginAdapter.mainLoop();
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Plugin threw an exception at top level: " << ex.what());
        ret = 40;
    }
    LOGINFO("Plugin Finished.");
    return ret;
}
