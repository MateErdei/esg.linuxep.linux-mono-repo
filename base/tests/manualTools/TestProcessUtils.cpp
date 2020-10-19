/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <iostream>
#include <Common/ProcessUtils/ProcUtilities.h>
#include <Common/Logging/ConsoleLoggingSetup.h>

int main(int argc, char * argv[])
{

    Common::Logging::ConsoleLoggingSetup consoleSetup;

    if (argc != 3)
    {
        std::cout << "USAGE: ./TestProcessUtils <partOfComm> <requiresFullPathContainsPath>";
        return 1;
    }
    std::cout << argv[1] << std::endl << argv[2] << std::endl;
    ProcessUtils::ensureNoExecWithThisCommIsRunning(argv[1], argv[2]);
}
