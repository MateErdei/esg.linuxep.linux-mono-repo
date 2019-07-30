/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemCommands.h"

#include "Strings.h"

#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessException.h>
#include <Common/UtilityImpl/StrError.h>
#include <Common/UtilityImpl/TimeUtils.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>

namespace diagnose
{
    const int GL_10mbSize = 10 * 1024 * 1024;
    const int GL_ProcTimeoutMilliSecs = 500;
    const int GL_ProcMaxRetries = 10;

    using namespace Common::FileSystem;

    SystemCommands::SystemCommands(const std::string& destination) :
            m_destination(destination) {}

    int SystemCommands::runCommand(
        const std::string& command,
        std::vector<std::string> arguments,
        const std::string& filename)
    {
        Path filePath = Common::FileSystem::join(m_destination, filename);
        std::cout << "Output file path: " << filePath << std::endl;

        try
        {
            std::string exePath = getExecutablePath(command);
            auto output = runCommandOutputToString(exePath, arguments);
            fileSystem()->writeFile(filePath, output);
            return EXIT_SUCCESS;
        }
        catch (std::invalid_argument& e)
        {
            std::cout << command << " executable not found." << std::endl;
            fileSystem()->writeFile(filePath, e.what());
        }
        catch (Common::Process::IProcessException& e)
        {
            std::cout << "running process failed with error: " << e.what() << std::endl;
            fileSystem()->writeFile(filePath, e.what());
        }

        return EXIT_FAILURE;
    }

    std::string SystemCommands::runCommandOutputToString(const std::string& command, std::vector<std::string> args)
    {
        std::string commandAndArgs(command);
        std::for_each(
            args.begin(), args.end(), [&commandAndArgs](const std::string& arg) { commandAndArgs.append(" " + arg); });
        std::cout << "Running: " << commandAndArgs << std::endl;

        auto processPtr = Common::Process::createProcess();
        processPtr->setOutputLimit(GL_10mbSize);

        processPtr->exec(command, args);
        if (processPtr->wait(Common::Process::milli(GL_ProcTimeoutMilliSecs), GL_ProcMaxRetries) !=
            Common::Process::ProcessStatus::FINISHED)
        {
            processPtr->kill();
            std::stringstream ssTimeoutMessage;
            ssTimeoutMessage << "Process execution timed out after" << (GL_ProcTimeoutMilliSecs * GL_ProcMaxRetries)
                             << "running: '" << commandAndArgs << "'";
            throw Common::Process::IProcessException(ssTimeoutMessage.str());
        }

        auto output = processPtr->output();
        int exitCode = processPtr->exitCode();
        if (exitCode != 0)
        {
            throw Common::Process::IProcessException(
                "Process execution returned non-zero exit code, 'Exit Code: " +
                Common::UtilityImpl::StrError(exitCode) + "', Output: '" + output + "'");
        }
        return output;
    }

    std::string SystemCommands::getExecutablePath(std::string executableName)
    {
        std::vector<std::string> folderLocations = {"/usr/bin", "/bin", "/usr/local/bin", "/sbin", "/usr/sbin"};
        for (const auto& folder:folderLocations)
        {
            Path path = Common::FileSystem::join(folder,executableName);
            if( fileSystem()->isExecutable(path))
            {
                return path;
            }
        }
        throw std::invalid_argument("Executable " + executableName + " is not installed.");
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

        if( !  fileSystem()->isFile(tarfile) )
        {
            throw std::invalid_argument("tar file " + tarfile + " was not created");
        }
        std::cout << "Created tarfile: " << tarfileName << " in directory " << destPath << std::endl;
    }

} // namespace diagnose
