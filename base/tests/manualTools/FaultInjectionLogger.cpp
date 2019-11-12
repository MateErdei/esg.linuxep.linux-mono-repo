/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/Logging/LoggerConfig.h>
#include <Common/Logging/SophosLoggerMacros.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>

/**
 * Setup this executable to answer the following question:
 *  Our logs will ever fill up disk? Are they limited.
 *  Answer: Executed this test with options 100000000 1000000
 *  And only ever 11 files were created and 'old logs' were removed to give space for the new ones.
 */

int printUsageAndExit()
{
    std::cerr << "Usage: LoggerLimit <numLines> <pauseAfterN>\n"
                 "WIP \n";
    return 1;
}

void faultInjectionLoggingSetup(const std::string& logfilePath)
{
    Common::Logging::FileLoggingSetup::setupFileLoggingWithPath(logfilePath);
    Common::Logging::applyGeneralConfig("watchdog");
}

void logToLimit(log4cplus::Logger logger, unsigned int limit)
{
    for(unsigned long i =0; i < limit; i++)
    {
        std::stringstream st;
        st << "Write this boring line";
        st << i;
        LOG4CPLUS_INFO(logger, st.str());
    }
}

void logForever(log4cplus::Logger logger)
{
    LOG4CPLUS_INFO(logger, "Logging forever");
    unsigned int i = 0;
    while(true)
    {
        std::stringstream st;
        st << "Write this boring line";
        st << i++;
        LOG4CPLUS_INFO(logger, st.str());
    }  // NOLINT
}



int main(int argc, char * argv[])
{
    if ( argc != 3)
    {
        return printUsageAndExit();
    }
    unsigned long numLines = 1;
    std::string logPath;

    std::stringstream num;
    num.str(argv[1]);
    num >> numLines;
    logPath = argv[2];
    std::cout << "Options: numLines: " << numLines << std::endl;
    std::cout << "Options: logPath: " << logPath << std::endl;

    faultInjectionLoggingSetup(logPath);

    auto logger = Common::Logging::getInstance("watchdog");
    if (numLines == 0)
    {
        logForever(logger);
    }
    else
    {
        logToLimit(logger, numLines);
    }

    return 0;
}