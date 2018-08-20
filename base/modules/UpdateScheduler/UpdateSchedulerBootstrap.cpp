//
// Created by pair on 07/08/18.
//

#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <iostream>
#include "UpdateSchedulerBootstrap.h"
#include "SchedulerTaskQueue.h"
#include "SchedulerPluginCallback.h"
#include "UpdateScheduler.h"
#include "Logger.h"
#include "LoggingSetup.h"

log4cplus::Logger GL_UPDSCH_LOGGER; //NOLINT

namespace UpdateScheduler
{
    int main_entry()
    {
        LoggingSetup logging;

        std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement = Common::PluginApi::createPluginResourceManagement();

        std::shared_ptr<SchedulerTaskQueue> queueTask = std::make_shared<SchedulerTaskQueue>();
        auto sharedPluginCallBack = std::make_shared<SchedulerPluginCallback>(queueTask);

        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService = resourceManagement->createPluginAPI(
                "UpdateScheduler", sharedPluginCallBack
        );
        UpdateScheduler updateScheduler(queueTask, std::move(baseService), sharedPluginCallBack);
        updateScheduler.mainLoop();
        LOGINFO("Update Scheduler Finished.");
        return 0;
    }
}