// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/Main/Main.h"

#include "ResponseActions/ActionRunner/responseaction_main.h"

static int action_runner_main(int argc, char* argv[])
{
    return ActionRunner::responseaction_main::main(argc, argv);
}

MAIN(action_runner_main(argc, argv))