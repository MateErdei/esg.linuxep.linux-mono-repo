/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/Main.h>
#include <Diagnose/diagnose/diagnose_main.h>

static int diagnose_main(int )
{
    return diagnose::diagnose_main::main();
}

MAIN(diagnose_main(argc))