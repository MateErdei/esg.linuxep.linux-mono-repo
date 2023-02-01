/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/PluginLoggingSetup.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/ErrorCodes.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <ResponseActions/pluginimpl/Logger.h>
#include <ResponseActions/pluginimpl/PluginAdapter.h>
#include <pluginimpl/config.h>

#include <sstream>

const char* PluginName = RA_PLUGIN_NAME;

enum returnCode {
    SUCCESS = 0,
    EXCEPTIONTHROWN = 40,
};

int main()
{
    using namespace Plugin;
    int ret = SUCCESS;
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
        std::stringstream errorMsg;
        errorMsg << "Plugin Api could not be instantiated: " << apiException.what();
        LOGERROR(errorMsg.str());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), sharedPluginCallBack);

    try
    {
        pluginAdapter.mainLoop();
    }
    catch (const DetectRequestToStop& detect)
    {
        LOGINFO(detect.what());
        ret = SUCCESS;
    }
    catch (const std::exception& ex)
    {
        std::stringstream errorMsg;
        errorMsg << "Plugin threw an exception at top level: " << ex.what();
        LOGERROR(errorMsg.str());
        ret = EXCEPTIONTHROWN;
    }
    sharedPluginCallBack->setRunning(false);
    LOGINFO("Plugin Finished.");
    return ret;
}
