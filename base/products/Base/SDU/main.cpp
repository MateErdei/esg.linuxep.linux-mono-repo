// Copyright 2021-2024 Sophos Limited. All rights reserved.
#include "sdu/Main.h"

#include "Common/Main/Main.h"

static int remote_diagnose_main()
{
    return RemoteDiagnoseImpl::main_entry();
}

MAIN(remote_diagnose_main());
