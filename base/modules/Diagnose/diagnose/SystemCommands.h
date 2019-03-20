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
        
        void runCommandOutputToFile(std::string command, std::string filename);
        std::string exec(const std::string& cmd);
    private:
        std::string m_destination;
        Common::FileSystem::FileSystemImpl m_fileSystem;

    };

}

