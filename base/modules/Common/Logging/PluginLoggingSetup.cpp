/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "PluginLoggingSetup.h"
#include "FileLoggingSetup.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

#include <log4cplus/logger.h>

Common::Logging::PluginLoggingSetup::PluginLoggingSetup(const std::string& pluginName)
{
    setupFileLogging(pluginName);
}


Common::Logging::PluginLoggingSetup::~PluginLoggingSetup()
{
    log4cplus::Logger::shutdown();
}

void Common::Logging::PluginLoggingSetup::setupFileLogging(const std::string& pluginName)
{

    std::string logPath = Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().sophosInstall(),
            "plugins",
            pluginName);
    logPath = Common::FileSystem::join(logPath, "log",
                                       pluginName + ".log");
    Common::Logging::FileLoggingSetup::setupFileLoggingWithPath(logPath);
}
