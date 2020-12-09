/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "PluginLoggingSetup.h"

#include "FileLoggingSetup.h"
#include "LoggerConfig.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <log4cplus/logger.h>

Common::Logging::PluginLoggingSetup::PluginLoggingSetup(
    const std::string& pluginName,
    const std::string& overrideFileName)
{
    std::string fileName{ overrideFileName };
    if (fileName.empty())
    {
        fileName = pluginName;
    }
    setupFileLogging(pluginName, fileName);
    applyGeneralConfig(fileName);
}

Common::Logging::PluginLoggingSetup::~PluginLoggingSetup()
{
    log4cplus::Logger::shutdown();
}

void Common::Logging::PluginLoggingSetup::setupFileLogging(
    const std::string& pluginNameDir,
    const std::string& pluginFileName)
{
    std::string logPath = Common::FileSystem::join(
        Common::ApplicationConfiguration::applicationPathManager().sophosInstall(), "plugins", pluginNameDir);
    logPath = Common::FileSystem::join(logPath, "log", pluginFileName + ".log");
    Common::Logging::FileLoggingSetup::setupFileLoggingWithPath(logPath);
}
