/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemCommands.h"
#include "Strings.h"

#include <Common/FileSystem/IFileSystemException.h>

#include <iostream>

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

    void SystemCommands::cleanupDir(const std::string& dirPath)
    {
        std::string removeCommand = "rm -rf " + dirPath;
        int ret = system(removeCommand.c_str());
        if (ret != 0)
        {
            std::cout << "Error: " << removeCommand << " failed." << std::endl;
        }
    }

    void SystemCommands::cleanupDirs(const std::string& dirPath)
    {
        cleanupDir(Common::FileSystem::join(dirPath, PLUGIN_FOLDER));
        cleanupDir(Common::FileSystem::join(dirPath, BASE_FOLDER));
        cleanupDir(Common::FileSystem::join(dirPath, SYSTEM_FOLDER));
    }

    void SystemCommands::tarDiagnoseFolder(const std::string& dirPath)
    {
        std::string tarfile = Common::FileSystem::join(dirPath, "sspl-diagnose.tar.gz");
        std::cout << "Running tar on: " << dirPath <<std::endl;

        std::string tarCommand = "tar -czf " + tarfile + " -C " + dirPath + " " + PLUGIN_FOLDER + " " + BASE_FOLDER + " " + SYSTEM_FOLDER;

        Common::FileSystem::FileSystemImpl fileSystem;

        int ret =  system(tarCommand.c_str());
        if (ret != 0)
        {
            throw std::invalid_argument("tar file command failed");
        }

        if(fileSystem.isFile(tarfile) )
        {
            if(isSafeToDelete(dirPath))
            {
                cleanupDirs(dirPath);
            }
            else
            {
                throw std::invalid_argument("dirpath is not safe to delete: " + dirPath);
            }
        }
        else
        {
            throw std::invalid_argument("tar file " + tarfile + " was not created");
        }
    }

    bool SystemCommands::isSafeToDelete(const std::string& path)
    {
        return !(Common::FileSystem::basename(path) != "DiagnoseOutput");

    }

} // namespace diagnose
