// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "Common/Main/Main.h"

#include "UpdateSchedulerImpl/UpdateSchedulerBootstrap.h"

static int update_scheduler_main()
{
    return UpdateSchedulerImpl::main_entry();
}

MAIN(update_scheduler_main());
