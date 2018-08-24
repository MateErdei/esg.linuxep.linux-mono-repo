///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#include <watchdog/watchdogimpl/watchdog_main.h>

#include <Common/UtilityImpl/Main.h>

int watchdog_main(int argc, char* argv[])
{
    return watchdog::watchdogimpl::watchdog_main::main(argc,argv);
}

MAIN(watchdog_main(argc,argv))
