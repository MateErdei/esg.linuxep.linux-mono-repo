/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "wdctl_main.h"

#include "wdctl_bootstrap.h"

int wdctl::wdctlimpl::wdctl_main(int argc, char** argv)
{
    wdctl_bootstrap boot;
    return boot.main(argc, argv);
}
