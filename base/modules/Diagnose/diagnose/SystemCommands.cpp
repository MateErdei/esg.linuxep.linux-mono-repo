/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "SystemCommands.h"
#include <stdlib.h>

namespace diagnose
{
    void SystemCommands::runCommand(std::string command)
    {
        int ret = system(command.c_str());
        static_cast<void>(ret);
    }
}
