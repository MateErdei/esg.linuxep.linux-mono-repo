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
        int runCommand(std::string command, std::string filename);

    private:
        std::string m_destination;
        Common::FileSystem::FileSystemImpl m_fileSystem;
    };
} // namespace diagnose
