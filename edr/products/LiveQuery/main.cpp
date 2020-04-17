/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/Main.h>


static int query_runner_main(int argc, char* argv[])
{
    return diagnose::diagnose_main::main(argc, argv);
}

MAIN(query_runner_main(argc, argv))