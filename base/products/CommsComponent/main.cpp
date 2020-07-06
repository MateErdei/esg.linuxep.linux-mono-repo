/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/Main.h>
#include <CommsComponent/Comms.h>

static int comms_component_main()
{
    return CommsComponent::main_entry();
}

MAIN(comms_component_main())
