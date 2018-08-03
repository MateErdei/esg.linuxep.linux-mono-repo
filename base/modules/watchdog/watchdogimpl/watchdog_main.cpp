/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "watchdog_main.h"

#include "Logger.h"
#include "Watchdog.h"

using namespace watchdog::watchdogimpl;

/**
 * Static method called from watchdog executable
 * @param argc
 * @param argv
 * @return
 */
int watchdog_main::main(int argc, char **argv)
{
    static_cast<void>(argv); // unused
    if(argc > 1)
    {
        LOGERROR("Error, invalid command line arguments. Usage: watchdog");
        return 2;
    }

    Watchdog m;
    return m.run();
}
