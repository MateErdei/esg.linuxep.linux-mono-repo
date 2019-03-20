/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace diagnose
{

    class SystemCommands
    {


    public:
        SystemCommands() = default;
        
        void runCommand(std::string command);

    };

}

