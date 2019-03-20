/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemCommands.h"

#include <Common/FileSystem/IFileSystemException.h>

#include <stdlib.h>

namespace diagnose
{
    SystemCommands::SystemCommands(std::string destination) : m_destination(destination) {}

    int SystemCommands::runCommand(std::string command, std::string filename)
    {
        Path filePath = Common::FileSystem::join(m_destination, filename);
        std::string fullCommand = command + " >" + filePath + " 2>&1";

        return system(fullCommand.c_str());
    }

    void SystemCommands::copyFile(std::string filename, std::string destination)
    {
        if (m_fileSystem.isFile(filename))
        {
            m_fileSystem.copyFile(filename, destination);
        }
    }
} // namespace diagnose
