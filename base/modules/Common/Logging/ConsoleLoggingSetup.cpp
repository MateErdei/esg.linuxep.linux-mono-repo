/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ConsoleLoggingSetup.h"
#include "LoggerConfig.h"
#include "LoggingSetup.h"
#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <log4cplus/configurator.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

namespace
{
    class NoLogConfigPathManager : public Common::ApplicationConfigurationImpl::ApplicationPathManager
    {
    public:
        using Common::ApplicationConfigurationImpl::ApplicationPathManager::ApplicationPathManager;
        std::string getLogConfFilePath() const override
        {
            return std::string{};
        }
    };

}


using namespace Common::ApplicationConfigurationImpl;
using namespace Common::ApplicationConfiguration;



Common::Logging::ConsoleLoggingSetup::ConsoleLoggingSetup()
{
    consoleSetupLogging();
    applyGeneralConfig( Common::Logging::LOGFORTEST);
}

Common::Logging::ConsoleLoggingSetup::~ConsoleLoggingSetup()
{
    log4cplus::Logger::shutdown();
}

void Common::Logging::ConsoleLoggingSetup::consoleSetupLogging()
{
    log4cplus::initialize();

    log4cplus::SharedAppenderPtr appender(new log4cplus::ConsoleAppender(true));
    Common::Logging::LoggingSetup::applyPattern(appender, "%m%n");

    log4cplus::Logger::getRoot().addAppender(appender);
}
