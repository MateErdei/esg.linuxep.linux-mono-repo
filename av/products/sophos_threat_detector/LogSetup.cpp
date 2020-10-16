/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LogSetup.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Logging/FileLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"
#include "../common/config.h"

#include <log4cplus/logger.h>

LogSetup::LogSetup()
{
    std::string logfilepath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    logfilepath += "/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log";

    Common::Logging::FileLoggingSetup::setupFileLoggingWithPath(logfilepath);
    Common::Logging::applyGeneralConfig(PLUGIN_NAME);
}

LogSetup::~LogSetup()
{
    log4cplus::Logger::shutdown();
}
