/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemCommands.h"

#include <Common/FileSystem/IFileSystemException.h>

#include <iostream>
#include <stdlib.h>

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

    int SystemCommands::tarDiagnoseFolder(const std::string& dirPath)
    {

        std::string tarfile = Common::FileSystem::join(dirPath, "sspl-diagnose.tar.gz");
        std::cout << "Running tar on: " << dirPath <<std::endl;

        std::string tarCommand = "tar -czf " + tarfile + " -C " + dirPath + " PluginFiles BaseFiles SystemFiles";

        Common::FileSystem::FileSystemImpl fileSystem;

        int ret =  system(tarCommand.c_str());
        //check ret

        if(fileSystem.isFile(tarfile) )
        {
            if(isSafeToDelete(dirPath))
            {
                std::string removeBaseFilesCommand = "rm -rf " + Common::FileSystem::join(dirPath,"BaseFiles");
                std::string removeSystemFilesCommand = "rm -rf " + Common::FileSystem::join(dirPath,"SystemFiles");
                std::string removePluginsFilesCommand = "rm -rf " + Common::FileSystem::join(dirPath,"PluginFiles");
                int ret = system(removeBaseFilesCommand.c_str());
                ret = system(removeSystemFilesCommand.c_str());
                ret = system(removePluginsFilesCommand.c_str());
                static_cast<void>(ret);
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
        return ret;
    }

    bool SystemCommands::isSafeToDelete(const std::string& path)
    {
        return !(Common::FileSystem::basename(path) != "DiagnoseOutput");

    }

} // namespace diagnose
