/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/FileSystemImpl/FileSystemImpl.h>

#include <string>

namespace diagnose
{
    class SystemCommands
    {
    public:
        explicit SystemCommands(std::string destination);

        /*
         * runs a command and writes the output to a file
         */
        int runCommand(const std::string& command, const std::string& filename);

    private:
        std::string m_destination;
    };
} // namespace diagnose
