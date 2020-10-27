/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/Main.h>
#include <modules/liveexecutable/livequery_main.h>

static int query_runner_main(int argc, char* argv[])
{
    return livequery::livequery_main::main(argc, argv);
}

MAIN(query_runner_main(argc, argv))