// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "liveexecutable/livequery_main.h"

#include "Common/Main/Main.h"

static int query_runner_main(int argc, char* argv[])
{
    return livequery::livequery_main::main(argc, argv);
}

MAIN(query_runner_main(argc, argv))