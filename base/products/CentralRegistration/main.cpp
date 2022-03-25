/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Main.h"

#include <Common/UtilityImpl/Main.h>

static int central_registration_main()
{
    return CentralRegistrationImpl::main_entry();
}

MAIN(central_registration_main());
