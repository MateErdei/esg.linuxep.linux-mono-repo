// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "UpdateSchedulerBootstrap.h"

#include "SchedulerPluginCallback.h"
#include "UpdateSchedulerProcessor.h"
#include "UpdateSchedulerUtils.h"

#include "common/Logger.h"
#include "cronModule/CronSchedulerThread.h"
#include "runnerModule/AsyncSulDownloaderRunner.h"

// Auto version headers
#include "AutoVersioningHeaders/AutoVersion.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Logging/FileLoggingSetup.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"
#include "Common/UtilityImpl/UniformIntDistribution.h"
#include "UpdateScheduler/SchedulerTaskQueue.h"

#include <sys/stat.h>

#include <iostream>

namespace UpdateSchedulerImpl
{
    using namespace UpdateScheduler;
    int main_entry()
    {
        umask(S_IRWXG | S_IRWXO); // Read and write for the owner
        Common::Logging::FileLoggingSetup logging("updatescheduler", true);
        LOGINFO("Update Scheduler " << _AUTOVER_COMPONENTAUTOVERSION_STR_ << " started");

        auto resourceManagement = Common::PluginApi::createPluginResourceManagement();
        auto taskQueue = std::make_shared<SchedulerTaskQueue>();
        auto sharedPluginCallBack = std::make_shared<SchedulerPluginCallback>(taskQueue);
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;
        try
        {
            baseService = resourceManagement->createPluginAPI("updatescheduler", sharedPluginCallBack);
        }
        catch (const Common::PluginApi::ApiException& apiException)
        {
            LOGERROR("ApiException creating Plugin API");
            log_exception(apiException);
            return 1;
        }

        // on start up UpdateScheduler must perform an upgrade between 5 and 10 minutes (300 seconds, 600 seconds)
        Common::UtilityImpl::UniformIntDistribution distribution(300, 600);

        auto cronThread = std::make_unique<cronModule::CronSchedulerThread>(
            taskQueue, std::chrono::seconds(distribution.next()), std::chrono::minutes(60));
        auto dirPath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath();
        auto runner = std::make_unique<runnerModule::AsyncSulDownloaderRunner>(taskQueue, dirPath);

        {
            UpdateSchedulerProcessor updateScheduler(
                std::move(taskQueue),
                std::move(baseService),
                sharedPluginCallBack,
                std::move(cronThread),
                std::move(runner));
            updateScheduler.mainLoop();
            sharedPluginCallBack->setRunning(
                false); // Needs to be set to false before UpdateSchedulerProcessor is deleted
        }
        sharedPluginCallBack.reset();
        resourceManagement.reset();
        LOGINFO("Update Scheduler Finished.");
        return 0;
    }
} // namespace UpdateSchedulerImpl