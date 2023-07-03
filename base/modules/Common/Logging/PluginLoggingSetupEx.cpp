/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "PluginLoggingSetupEx.h"

#include "FileLoggingSetupEx.h"
#include "LoggerConfig.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include <log4cplus/logger.h>

Common::Logging::PluginLoggingSetupEx::PluginLoggingSetupEx(
        const std::string& pluginName,
        const std::string& overrideFileName,
        const std::string& instanceName)
{
    std::string fileName(overrideFileName);
    std::string logInstanceName(instanceName);

    if (fileName.empty())
    {
        fileName = pluginName;
    }
    if(logInstanceName.empty())
    {
        logInstanceName = pluginName;
    }

    setupFileLogging(pluginName, fileName, instanceName);
    applyGeneralConfig(fileName);
}

Common::Logging::PluginLoggingSetupEx::~PluginLoggingSetupEx()
{
    log4cplus::Logger::shutdown();
}

void Common::Logging::PluginLoggingSetupEx::setupFileLogging(
        const std::string& pluginNameDir,
        const std::string& pluginFileName,
        const std::string& instanceName)
{
    std::string logPath = Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().sophosInstall(), "plugins", pluginNameDir);
    logPath = Common::FileSystem::join(logPath, "log", pluginFileName + ".log");
    Common::Logging::FileLoggingSetupEx::setupFileLoggingWithPath(logPath, instanceName);
}