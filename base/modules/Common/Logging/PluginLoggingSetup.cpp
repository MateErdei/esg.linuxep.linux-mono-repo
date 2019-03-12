/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "PluginLoggingSetup.h"

#include "FileLoggingSetup.h"
#include "LoggerConfig.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <log4cplus/logger.h>

Common::Logging::PluginLoggingSetup::PluginLoggingSetup(const std::string& pluginName)
{
    setupFileLogging(pluginName);
    applyGeneralConfig(pluginName);
}

Common::Logging::PluginLoggingSetup::~PluginLoggingSetup()
{
    log4cplus::Logger::shutdown();
}

void Common::Logging::PluginLoggingSetup::setupFileLogging(const std::string& pluginName)
{
    std::string logPath = Common::FileSystem::join(
        Common::ApplicationConfiguration::applicationPathManager().sophosInstall(), "plugins", pluginName);
    logPath = Common::FileSystem::join(logPath, "log", pluginName + ".log");
    Common::Logging::FileLoggingSetup::setupFileLoggingWithPath(logPath);
}
