/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemCommands.h"
#include "Strings.h"

#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/TimeUtils.h>

#include <iostream>
#include <algorithm>

namespace diagnose
{
    SystemCommands::SystemCommands(const std::string& destination) : m_destination(destination) {}

    int SystemCommands::runCommand(const std::string& command, const std::string& filename)
    {
        std::cout << "Running: " << command << ", output to: " << filename << std::endl;
        Path filePath = Common::FileSystem::join(m_destination, filename);
        std::string fullCommand = command + " >'" + filePath + "' 2>&1";
        return system(fullCommand.c_str());
    }


    void SystemCommands::tarDiagnoseFolder(const std::string& srcPath, const std::string& destPath)
    {
        Common::UtilityImpl::FormattedTime m_formattedTime;
        Common::FileSystem::FileSystemImpl fileSystem;

        std::cout << "Running tar on: " << srcPath <<std::endl;

        std::string timestamp = m_formattedTime.currentTime();
        std::replace(timestamp.begin(), timestamp.end(), ' ', '_');
        std::string tarfileName = "sspl-diagnose_" + timestamp + ".tar.gz";

        std::string tarfile = Common::FileSystem::join(destPath, tarfileName);

        std::string tarCommand = "tar -czf " + tarfile + " -C '" + srcPath + "' " + PLUGIN_FOLDER + " " + BASE_FOLDER + " " + SYSTEM_FOLDER;

        int ret =  system(tarCommand.c_str());
        if (ret != 0)
        {
            throw std::invalid_argument("tar file command failed");
        }

        if( ! fileSystem.isFile(tarfile) )
        {
            throw std::invalid_argument("tar file " + tarfile + " was not created");
        }
        std::cout << "Created tarfile: " << tarfileName << " in directory " << destPath << std::endl;
    }

} // namespace diagnose
