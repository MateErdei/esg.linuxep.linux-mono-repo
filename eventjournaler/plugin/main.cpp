/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"
#include "Logger.h"
#include "config.h"

#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/FileLoggingSetup.h>

const char * PluginName = PLUGIN_NAME;



int main()
{
    using namespace TemplatePlugin;
    std::string logPath = Common::FileSystem::join(Common::PluginApi::getInstallRoot(), "plugins");
    logPath = Common::FileSystem::join(logPath, PluginName);
    logPath = Common::FileSystem::join(logPath, "log");
    logPath = Common::FileSystem::join(logPath, std::string(PluginName) + ".log");

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
