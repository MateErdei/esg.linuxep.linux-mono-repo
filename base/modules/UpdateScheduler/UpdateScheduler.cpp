//
// Created by pair on 07/08/18.
//

#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <iostream>
#include "UpdateScheduler.h"
#include "QueueTask.h"
#include "PluginCallback.h"
#include "PluginAdapter.h"
#include "Logger.h"
#include "LoggingSetup.h"

log4cplus::Logger GL_UPDSCH_LOGGER; //NOLINT

namespace UpdateScheduler
{
    int main_entry()
    {
        LoggingSetup logging;

        std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement = Common::PluginApi::createPluginResourceManagement();

        std::shared_ptr<QueueTask> queueTask = std::make_shared<QueueTask>();
        auto sharedPluginCallBack = std::make_shared<PluginCallback>(queueTask);

        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService = resourceManagement->createPluginAPI(
                "UpdateScheduler", sharedPluginCallBack
        );
        PluginAdapter pluginAdapter(queueTask, std::move(baseService), sharedPluginCallBack);
        pluginAdapter.mainLoop();
        LOGINFO("Update Scheduler Finished.");
        return 0;
    }
}