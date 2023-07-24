/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Scheduler.h"

#include "NoPolicyAvailableException.h"
#include "PluginCallback.h"
#include "SchedulerProcessor.h"
#include "SchedulerTask.h"
#include "TaskQueue.h"

#include "ZeroMQWrapper/IIPCException.h"

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/TaskQueue/ITaskQueue.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>
#include <sys/stat.h>

namespace TelemetrySchedulerImpl
{
    int main_entry()
    {
        try
        {
            umask(S_IRWXG | S_IRWXO | S_IXUSR); // Read and write for the owner
            Common::Logging::FileLoggingSetup loggerSetup("tscheduler", true);

            LOGINFO("Telemetry Scheduler running...");

            std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement =
                Common::PluginApi::createPluginResourceManagement();

            auto taskQueue = std::make_shared<TaskQueue>();
            auto pluginCallBack = std::make_shared<PluginCallback>(taskQueue);
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService =
                resourceManagement->createPluginAPI("tscheduler", pluginCallBack);

            Common::ApplicationConfigurationImpl::ApplicationPathManager pathManager;

            SchedulerProcessor telemetrySchedulerProcessor(taskQueue, pathManager);
//            taskQueue->push(SchedulerTask::InitialWaitToRunTelemetry);
            LOGINFO("Waiting for ALC policy before running Telemetry");
            try
            {
                baseService->requestPolicies("ALC");
            }
            catch (const Common::PluginApi::NoPolicyAvailableException&)
            {
                LOGINFO("No ALC policy available right now");
                // Ignore no Policy Available errors
            }
            catch (const Common::ZeroMQWrapper::IIPCException& exception)
            {
                LOGERROR("Failed to request ALC policy with error: "<< exception.what());
            }

//            bool newInstall = !(Common::FileSystem::fileSystem()->isFile(m_pathManager.getTelemetrySchedulerStatusFilePath()));

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
