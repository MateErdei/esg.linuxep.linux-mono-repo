/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "IBaseServiceApi.h"
#include "IPluginResourceManagement.h"
#include "FileSystem.h"
#include "PluginAdapter.h"
#include "Logger.h"
#include <LoggingAPI/FileLoggingSetup.h>

const char * PluginName = "ExamplePlugin";



int main()
{
    using namespace Example;
    FileSystem f = FileSystem();
    std::string logPath = f.join(Common::PluginApi::getInstallRoot(), "plugins");
    logPath = f.join(logPath, PluginName);
    logPath = f.join(logPath, "log");
    logPath = f.join(logPath, std::string(PluginName) + ".log");

    LoggingAPI::FileLoggingSetup loggerSetup(logPath);
    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement = Common::PluginApi::createPluginResourceManagement();

    std::shared_ptr<QueueTask> queueTask = std::make_shared<QueueTask>();
    auto sharedPluginCallBack = std::make_shared<PluginCallback>(queueTask);

    std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService = resourceManagement->createPluginAPI(PluginName, sharedPluginCallBack);

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), sharedPluginCallBack);

    try
    {
        pluginAdapter.mainLoop();
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Plugin threw an exception at top level: "<<ex.what());
    }
    LOGINFO("Plugin Finished.");
}
