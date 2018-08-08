/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include "PluginAdapter.h"
#include "Logger.h"

int main()
{
    using namespace UpdateScheduler;

    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement = Common::PluginApi::createPluginResourceManagement();

    std::shared_ptr<QueueTask> queueTask = std::make_shared<QueueTask>();
    auto sharedPluginCallBack = std::make_shared<PluginCallback>(queueTask);

    std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService = resourceManagement->createPluginAPI("UpdateScheduler", sharedPluginCallBack);

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), sharedPluginCallBack);

    pluginAdapter.mainLoop();
    LOGINFO("Plugin Finished.");
}