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
    std::cerr << "Usage: LoggerLimit <numLines> <pauseAfterN>\nnumLines: number of lines to write to log file. \n pauseAfterN: it will write the numLines in batches of pauseAfterN givin half second between batches.\n";
    return 1;
}


int main(int argc, char * argv[])
{
    if ( argc != 3)
    {
        return printUsageAndExit();
    }
    unsigned long numLines = 1;
    unsigned long pauseAfter = 1;
    std::stringstream num;
    num.str(argv[1]);
    num >> numLines;
    std::stringstream num2;
    num2.str(argv[2]);
    num2 >> pauseAfter;
    std::cout << "Options: numLines: " << numLines << std::endl;
    std::cout << "Options: pauseAfter: " << pauseAfter << std::endl;

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, "/tmp");

    Common::Logging::FileLoggingSetup setup("testlogging");

    auto logger = Common::Logging::getInstance("mytest");
    for(unsigned long i =0; i < numLines; i++  )
    {
        if( (i > pauseAfter) && ((i % pauseAfter)==0) )
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::stringstream st;
        st << "my product info. line: ";
        st << i;
        LOG4CPLUS_INFO(logger, st.str());
    }

    return 0;
}