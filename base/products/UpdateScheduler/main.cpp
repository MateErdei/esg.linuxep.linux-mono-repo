/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "UpdateSchedulerBootstrap.h"

#include <Common/UtilityImpl/Main.h>

static int update_scheduler_main()
{
    return UpdateSchedulerImpl::main_entry();
}

MAIN(update_scheduler_main());
