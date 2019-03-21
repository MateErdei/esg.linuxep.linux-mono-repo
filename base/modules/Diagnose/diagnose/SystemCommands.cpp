/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemCommands.h"

#include <Common/FileSystem/IFileSystemException.h>

#include <stdlib.h>
#include <iostream>

namespace diagnose
{
    SystemCommands::SystemCommands(std::string destination) : m_destination(destination) {}

    int SystemCommands::runCommand(const std::string& command, const std::string& filename)
    {
        std::cout << "Running: " << command << ", output to: " << filename << std::endl;
        Path filePath = Common::FileSystem::join(m_destination, filename);
        std::string fullCommand = command + " >" + filePath + " 2>&1";
        return system(fullCommand.c_str());
    }

} // namespace diagnose
