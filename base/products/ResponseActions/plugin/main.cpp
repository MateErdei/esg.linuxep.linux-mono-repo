// Copyright 2023 Sophos Limited. All rights reserved.
#include "Common/Logging/PluginLoggingSetup.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApi/ErrorCodes.h"
#include "Common/PluginApi/IBaseServiceApi.h"
#include "Common/PluginApi/IPluginResourceManagement.h"
#include "ResponseActions/ResponsePlugin/ActionRunner.h"
#include "ResponseActions/ResponsePlugin/Logger.h"
#include "ResponseActions/ResponsePlugin/PluginAdapter.h"
#include "ResponseActions/ResponsePlugin/config.h"

#include <sstream>

const char* PluginName = RA_PLUGIN_NAME;

enum returnCode
{
    SUCCESS = 0,
    EXCEPTIONTHROWN = 40,
};

int main()
{
    using namespace ResponsePlugin;
    int ret = SUCCESS;
    Common::Logging::PluginLoggingSetup loggerSetup(PluginName);

    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement =
        Common::PluginApi::createPluginResourceManagement();

    auto queueTask = std::make_shared<TaskQueue>();
    auto sharedPluginCallBack = std::make_shared<PluginCallback>(queueTask);

    std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;
    auto runner = std::make_unique<ResponsePlugin::ActionRunner>(queueTask);

    try
    {
        baseService = resourceManagement->createPluginAPI(PluginName, sharedPluginCallBack);
    }
    catch (const Common::PluginApi::ApiException& apiException)
    {
        LOGERROR("Plugin Api could not be instantiated: " << apiException.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), std::move(runner), sharedPluginCallBack);

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
        LOGERROR("Plugin threw an exception at top level: " << ex.what());
        ret = EXCEPTIONTHROWN;
    }
    catch (...)
    {
        LOGERROR("Plugin threw an unknown exception at top level");
        ret = EXCEPTIONTHROWN;
    }
    sharedPluginCallBack->setRunning(false);
    LOGINFO("Plugin Finished.");
    return ret;
}
