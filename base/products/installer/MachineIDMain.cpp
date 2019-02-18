/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/OSUtilitiesImpl/SXLMachineID.h>
#include <Common/UtilityImpl/Main.h>

static int machineid_main(int argc, char* argv[])
{
    return Common::OSUtilitiesImpl::mainEntry(argc, argv);
}

MAIN(machineid_main(argc, argv));