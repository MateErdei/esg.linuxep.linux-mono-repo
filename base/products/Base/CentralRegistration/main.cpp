// Copyright 2022-2023 Sophos Limited. All rights reserved.
#include "CentralRegistration/Main.h"

#include "Common/Main/Main.h"

static int central_registration_main(int argc, char* argv[])
{
    return CentralRegistration::main_entry(argc, argv);
}

MAIN(central_registration_main(argc, argv))
