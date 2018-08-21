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
        std::unique_ptr<CronSchedulerThread> cronThread = std::unique_ptr<CronSchedulerThread>(new
                                                                                                       CronSchedulerThread(
                queueTask, std::chrono::minutes(10), std::chrono::minutes(60)));
        UpdateScheduler updateScheduler(queueTask, std::move(baseService), sharedPluginCallBack, std::move(cronThread));
        updateScheduler.mainLoop();
        LOGINFO("Update Scheduler Finished.");
        return 0;
    }
}