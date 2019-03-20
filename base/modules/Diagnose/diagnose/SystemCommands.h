/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <Common/FileSystemImpl/FileSystemImpl.h>

namespace diagnose
{
    class SystemCommands
    {

    public:
        explicit SystemCommands(std::string destination);
        
        int runCommand(std::string command, std::string filename);

    private:
        std::string m_destination;

    };
}
