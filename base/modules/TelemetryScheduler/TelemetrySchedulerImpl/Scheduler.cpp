/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Scheduler.h"

#include "PluginCallback.h"
#include "SchedulerProcessor.h"
#include "TaskQueue.h"

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/TaskQueue/ITaskQueue.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <stdexcept>

namespace TelemetrySchedulerImpl
{
    int main_entry()
    {
        try
        {
            Common::Logging::FileLoggingSetup loggerSetup("telemetryScheduler", true);

            LOGINFO("Telemetry Scheduler running...");

            std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement =
                Common::PluginApi::createPluginResourceManagement();

            auto taskQueue = std::make_shared<TaskQueue>();
            auto pluginCallBack = std::make_shared<PluginCallback>(taskQueue);
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService =
                resourceManagement->createPluginAPI("tscheduler", pluginCallBack);

            Common::ApplicationConfigurationImpl::ApplicationPathManager pathManager;

            SchedulerProcessor telemetrySchedulerProcessor(taskQueue, pathManager);
            taskQueue->push(Task::WaitToRunTelemetry); // initialise processor
            telemetrySchedulerProcessor.run();

            LOGINFO("Telemetry Scheduler finished");
        }
        catch (const std::runtime_error& e)
        {
            LOGERROR("Error: " << e.what());
            return 1;
        }

        return 0;
    }

} // namespace TelemetrySchedulerImpl
