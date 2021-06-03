/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Scheduler.h"

#include "PluginCallback.h"
#include "PluginAdapter.h"
#include "TaskQueue.h"
#include "Logger.h"
#include "IAsyncDiagnoseRunner.h"
#include "runnerModule/AsyncDiagnoseRunner.h"
#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/Logging/FileLoggingSetup.h>

#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/TaskQueue/ITaskQueue.h>

#include <stdexcept>
#include <sys/stat.h>

namespace RemoteDiagnoseImpl
{
    int main_entry()
    {
        try
        {
            umask(S_IRWXG | S_IRWXO | S_IXUSR); // Read and write for the owner
            Common::Logging::FileLoggingSetup loggerSetup("remote_diagnose", true);

            LOGINFO("SDU running...");

            std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement =
                Common::PluginApi::createPluginResourceManagement();

            auto taskQueue = std::make_shared<TaskQueue>();
            auto pluginCallBack = std::make_shared<PluginCallback>(taskQueue);
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService =
                resourceManagement->createPluginAPI("sdu", pluginCallBack);

            Common::ApplicationConfigurationImpl::ApplicationPathManager pathManager;

            std::string dirPath = pathManager.getDiagnoseOutputPath();
            std::unique_ptr<IAsyncDiagnoseRunner> runner =
                    std::unique_ptr<IAsyncDiagnoseRunner>(new runnerModule::AsyncDiagnoseRunner(taskQueue, dirPath));
            PluginAdapter pluginAdapter(
                    taskQueue, std::move(baseService), pluginCallBack, std::move(runner));
            pluginAdapter.mainLoop();
            //pluginCallBack->setRunning(false);

            LOGINFO("SDU finished");
        }
        catch (const std::runtime_error& e)
        {
            LOGERROR("Error: " << e.what());
            return 1;
        }

        return 0;
    }

} // namespace RemoteDiagnoseImpl
