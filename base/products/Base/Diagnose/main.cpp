// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "Common/Main/Main.h"

#include "Diagnose/diagnose/diagnose_main.h"

static int diagnose_main(int argc, char* argv[])
{
    return diagnose::diagnose_main::main(argc, argv);
}

MAIN(diagnose_main(argc, argv))