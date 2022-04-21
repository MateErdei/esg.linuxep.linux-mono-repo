/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Main.h"

#include <Common/UtilityImpl/Main.h>

static int central_registration_main(int argc, char* argv[])
{
    return CentralRegistration::main_entry(argc, argv);
}

MAIN(central_registration_main(argc, argv))
