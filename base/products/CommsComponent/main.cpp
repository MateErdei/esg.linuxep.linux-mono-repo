/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/Main.h>
#include <CommsComponent/Comms.h>

static int comms_component_man()
{
    return Comms::main_entry();
}

MAIN(comms_component_man())
