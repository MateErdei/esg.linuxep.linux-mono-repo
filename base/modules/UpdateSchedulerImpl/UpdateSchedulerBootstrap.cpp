//
// Created by pair on 07/08/18.
//

#include "Logger.h"
#include "LoggingSetup.h"
#include "SchedulerPluginCallback.h"
#include "UpdateSchedulerBootstrap.h"
#include "UpdateSchedulerProcessor.h"
#include "cronModule/CronSchedulerThread.h"
#include "runnerModule/AsyncSulDownloaderRunner.h"
#include <UpdateScheduler/SchedulerTaskQueue.h>

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/UtilityImpl/UniformIntDistribution.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/PluginApi/ApiException.h>

#include <iostream>

log4cplus::Logger GL_UPDSCH_LOGGER; //NOLINT

namespace UpdateSchedulerImpl
{
    using namespace UpdateScheduler;
    int main_entry()
    {
        LoggingSetup logging;

        std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement = Common::PluginApi::createPluginResourceManagement();

        std::shared_ptr<SchedulerTaskQueue> queueTask = std::make_shared<SchedulerTaskQueue>();
        auto sharedPluginCallBack = std::make_shared<SchedulerPluginCallback>(queueTask);
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;
        try
        {
            baseService = resourceManagement->createPluginAPI("updatescheduler", sharedPluginCallBack);
        }
        catch (const Common::PluginApi::ApiException& apiException)
        {
            LOGERROR(apiException.what());
            throw apiException;
        }

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