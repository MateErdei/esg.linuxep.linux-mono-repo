// Copyright 2021-2023 Sophos Limited. All rights reserved.
#include "Main.h"

#include "Common/UtilityImpl/Main.h"

static int update_scheduler_main()
{
    return RemoteDiagnoseImpl::main_entry();
}

MAIN(update_scheduler_main());
