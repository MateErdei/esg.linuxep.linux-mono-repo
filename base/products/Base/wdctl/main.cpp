// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/Main/Main.h"

#include "wdctl/wdctlimpl/wdctl_main.h"

static int wdctl_main(int argc, char* argv[])
{
    return wdctl::wdctlimpl::wdctl_main(argc, argv);
}

MAIN(wdctl_main(argc, argv))
