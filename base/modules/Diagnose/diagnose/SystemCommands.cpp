/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemCommands.h"
#include "Strings.h"

#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/Process/IProcess.h>

#include <iostream>
#include <algorithm>
#include <sstream>
#include <iterator>

namespace diagnose
{
    SystemCommands::SystemCommands(const std::string& destination) : m_destination(destination) {}

    int SystemCommands::runCommand(const std::string& commandInput, const std::string& filename)
    {
        auto process = Common::Process::createProcess();
        std::istringstream command(commandInput);

        std::vector<std::string> arguments{std::istream_iterator<std::string>{command},
                                        std::istream_iterator<std::string>{}};

        std::cout << "Running: " << commandInput << ", output to: " << filename << std::endl;
        Path filePath = Common::FileSystem::join(m_destination, filename);

        std::string base = arguments.at(0);
        arguments.erase(arguments.begin());

        try
        {
            std::string exePath = getExecutablePath(base);
            process->exec(exePath, arguments);
        }
        catch (std::invalid_argument)
        {
            std::cout << commandInput << " executable not found." << std::endl;
            m_fileSystem.writeFile(filePath,"Executable not found.");
            return 1;
        }

        m_fileSystem.writeFile(filePath,process->output());
        return process->exitCode();
    }

    std::string SystemCommands::getExecutablePath(std::string executableName)
    {
        std::vector<std::string> folderLocations = {"/usr/bin","/bin","/usr/local/bin"};
        for (auto folder:folderLocations)
        {
            Path path = Common::FileSystem::join(folder,executableName);
            if( m_fileSystem.isExecutable(path))
            {
                std::cout << "executable path: " << path << std::endl;
                return path;
            }
        }
        throw std::invalid_argument("executable is not installed");
    }

    void SystemCommands::tarDiagnoseFolder(const std::string& srcPath, const std::string& destPath)
    {
        Common::UtilityImpl::FormattedTime m_formattedTime;

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

        if( !  m_fileSystem.isFile(tarfile) )
        {
            throw std::invalid_argument("tar file " + tarfile + " was not created");
        }
        std::cout << "Created tarfile: " << tarfileName << " in directory " << destPath << std::endl;
    }

} // namespace diagnose
