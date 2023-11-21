// Copyright 2023 Sophos Limited. All rights reserved.

#ifdef SPL_BAZEL
#include "pluginimpl/config.h"
#else
#include "config.h"
#endif

#include "Common/Logging/PluginLoggingSetup.h"
#include "Common/Main/Main.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApi/ErrorCodes.h"
#include "Common/PluginApi/IBaseServiceApi.h"
#include "Common/PluginApi/IPluginResourceManagement.h"
#include "pluginimpl/Logger.h"
#include "pluginimpl/PluginAdapter.h"

static const char* PluginName = LR_PLUGIN_NAME;

static int inner_main()
{
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
    catch (const Common::PluginApi::ApiException& apiException)
    {
        LOGERROR("Plugin Api could not be instantiated: " << apiException.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), sharedPluginCallBack);

    try
    {
        pluginAdapter.mainLoop();
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Plugin threw an exception at top level: " << ex.what());
        ret = 40;
    }
    sharedPluginCallBack->setRunning(false);
    LOGINFO("Plugin Finished.");
    return ret;
}
MAIN(inner_main())
