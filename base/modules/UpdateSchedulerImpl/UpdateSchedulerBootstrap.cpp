//
// Created by pair on 07/08/18.
//

#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <iostream>
#include <Common/UtilityImpl/UniformIntDistribution.h>
#include "UpdateSchedulerBootstrap.h"
#include <UpdateScheduler/SchedulerTaskQueue.h>
#include <Common/Logging/FileLoggingSetup.h>
#include "SchedulerPluginCallback.h"
#include "cronModule/CronSchedulerThread.h"
#include "runnerModule/AsyncSulDownloaderRunner.h"
#include "UpdateSchedulerProcessor.h"
#include "Logger.h"

namespace UpdateSchedulerImpl
{
    using namespace UpdateScheduler;
    int main_entry()
    {
        Common::Logging::FileLoggingSetup logging("updatescheduler");

        std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement = Common::PluginApi::createPluginResourceManagement();

        std::shared_ptr<SchedulerTaskQueue> queueTask = std::make_shared<SchedulerTaskQueue>();
        auto sharedPluginCallBack = std::make_shared<SchedulerPluginCallback>(queueTask);

        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService = resourceManagement->createPluginAPI(
                "updatescheduler", sharedPluginCallBack
        );
        // on start up UpdateScheduler must perform an upgrade between 5 and 10 minutes (300 seconds, 600 seconds)
        Common::UtilityImpl::UniformIntDistribution distribution(300, 600);


        std::unique_ptr<ICronSchedulerThread> cronThread = std::unique_ptr<ICronSchedulerThread>(
                new cronModule::CronSchedulerThread(
                        queueTask, std::chrono::seconds(distribution.next()), std::chrono::minutes(60))
        );
        std::string dirPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath();
        std::unique_ptr<IAsyncSulDownloaderRunner> runner = std::unique_ptr<IAsyncSulDownloaderRunner>(
                new runnerModule::AsyncSulDownloaderRunner(queueTask, dirPath));

        UpdateSchedulerProcessor updateScheduler(queueTask, std::move(baseService), sharedPluginCallBack, std::move
                (cronThread), std::move(runner));
        updateScheduler.mainLoop();
        LOGINFO("Update Scheduler Finished.");
        return 0;
    }
}