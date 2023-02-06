/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/Main.h>
#include <Diagnose/diagnose/diagnose_main.h>

static int diagnose_main(int argc, char* argv[])
{
    return diagnose::diagnose_main::main(argc, argv);
}

MAIN(diagnose_main(argc, argv))