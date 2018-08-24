/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <wdctl/wdctlimpl/wdctl_main.h>

#include <Common/UtilityImpl/Main.h>

static int wdctl_main(int argc, char* argv[])
{
    return wdctl::wdctlimpl::wdctl_main(argc, argv);
}

MAIN(wdctl_main(argc,argv))
