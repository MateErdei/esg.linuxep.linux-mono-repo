// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/Main/Main.h"

#include "watchdog/watchdogimpl/watchdog_main.h"

static int watchdog_main(int argc, char* argv[])
{
    return watchdog::watchdogimpl::watchdog_main::main(argc, argv);
}

MAIN(watchdog_main(argc, argv))
