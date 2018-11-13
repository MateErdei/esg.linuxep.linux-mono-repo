/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include "FileSystem.h"
#include "PluginAdapter.h"
#include "Logger.h"
#include "config.h"

#include <Common/Logging/FileLoggingSetup.h>

const char * PluginName = PLUGIN_NAME;



int main()
{
    using namespace TemplatePlugin;
    FileSystem f = FileSystem();
    std::string logPath = f.join(Common::PluginApi::getInstallRoot(), "plugins");
    logPath = f.join(logPath, PluginName);
    logPath = f.join(logPath, "log");
    logPath = f.join(logPath, std::string(PluginName) + ".log");

    Common::Logging::FileLoggingSetup loggerSetup(logPath);
    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement = Common::PluginApi::createPluginResourceManagement();

    auto queueTask            = std::make_shared<QueueTask>();
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
