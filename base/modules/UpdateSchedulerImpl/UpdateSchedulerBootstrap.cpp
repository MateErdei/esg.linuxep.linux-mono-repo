/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "UpdateSchedulerBootstrap.h"

#include "Logger.h"
#include "SchedulerPluginCallback.h"
#include "UpdateSchedulerProcessor.h"

#include "cronModule/CronSchedulerThread.h"
#include "runnerModule/AsyncSulDownloaderRunner.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/UtilityImpl/UniformIntDistribution.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>
#include <sys/stat.h>

#include <iostream>

namespace UpdateSchedulerImpl
{
    using namespace UpdateScheduler;
    int main_entry()
    {
        umask(S_IRWXG | S_IRWXO); // Read and write for the owner
        Common::Logging::FileLoggingSetup logging("updatescheduler", true);

        std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement =
            Common::PluginApi::createPluginResourceManagement();

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
            return 1;
        }

        // on start up UpdateScheduler must perform an upgrade between 5 and 10 minutes (300 seconds, 600 seconds)
        Common::UtilityImpl::UniformIntDistribution distribution(300, 600);

        std::unique_ptr<ICronSchedulerThread> cronThread =
            std::unique_ptr<ICronSchedulerThread>(new cronModule::CronSchedulerThread(
                queueTask, std::chrono::seconds(distribution.next()), std::chrono::minutes(60)));
        std::string dirPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath();
        std::unique_ptr<IAsyncSulDownloaderRunner> runner =
            std::unique_ptr<IAsyncSulDownloaderRunner>(new runnerModule::AsyncSulDownloaderRunner(queueTask, dirPath));

        UpdateSchedulerProcessor updateScheduler(
            queueTask, std::move(baseService), sharedPluginCallBack, std::move(cronThread), std::move(runner));
        updateScheduler.mainLoop();
        LOGINFO("Update Scheduler Finished.");
        return 0;
    }
} // namespace UpdateSchedulerImpl