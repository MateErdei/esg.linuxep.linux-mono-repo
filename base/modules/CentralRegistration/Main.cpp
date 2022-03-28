/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Main.h"
#include "CentralRegistration.h"

#include "Logger.h"


namespace CentralRegistrationImpl
{
    int main_entry()
    {
        CentralRegistration centralRegistration;
        centralRegistration.RegisterWithCentral();
        return 0;
    }

} // namespace CentralRegistrationImpl
