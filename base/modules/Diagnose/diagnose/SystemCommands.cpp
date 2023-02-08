/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemCommands.h"

#include "Logger.h"
#include "Strings.h"
#include "SystemCommandException.h"

#include <Common/UtilityImpl/ProjectNames.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessException.h>
#include <Common/UtilityImpl/StrError.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/UtilityImpl/SystemExecutableUtils.h>
#include <Common/ZipUtilities/ZipUtils.h>

#include <algorithm>
#include <iostream>
#include <sys/stat.h>
#include <fstream>

namespace diagnose
{
    constexpr int GL_10mbSize = 10 * 1024 * 1024;
    const int GL_ProcTimeoutMilliSecs = 500;
    const int GL_ProcMaxRetries = 20;

    using namespace Common::FileSystem;

    SystemCommands::SystemCommands(const std::string& destination) : m_destination(destination) {}

    int SystemCommands::runCommand(
        const std::string& command,
        std::vector<std::string> arguments,
        const std::string& filename,
        const std::vector<u_int16_t>& exitcodes) const
    {
        Path filePath = Common::FileSystem::join(m_destination, filename);
        LOGINFO("Output file path: " << filePath);

        try
        {
            std::string exePath = Common::UtilityImpl::SystemExecutableUtils::getSystemExecutablePath(command);
            auto output = runCommandOutputToString(exePath, arguments,exitcodes);
            fileSystem()->writeFile(filePath, output);
            return EXIT_SUCCESS;
        }
        catch (std::invalid_argument& e)
        {
            LOGINFO(command << " executable not found.");
            fileSystem()->writeFile(filePath, e.what());
        }
        catch (SystemCommandsException& e)
        {
            std::stringstream commandString;
            commandString << command;
            for (size_t i = 0; i < arguments.size(); ++i)
            {
                commandString << " " << arguments[i];
            }
            LOGINFO("Running command: '" << commandString.str() << "' failed to complete with: " << e.what());

            std::stringstream message;
            message << e.output() << "***End Of Command Output***" << std::endl
                    << "Running command failed to complete with error: " << e.what();
            fileSystem()->writeFile(filePath, message.str());
        }

        return EXIT_FAILURE;
    }

    std::string SystemCommands::runCommandOutputToString(const std::string& command, std::vector<std::string> args, const std::vector<u_int16_t>& exitcodes)
        const
    {
        std::string commandAndArgs(command);
        std::for_each(
            args.begin(), args.end(), [&commandAndArgs](const std::string& arg) { commandAndArgs.append(" " + arg); });
        LOGINFO("Running: " << commandAndArgs);

        auto processPtr = Common::Process::createProcess();
        processPtr->setOutputLimit(GL_10mbSize);

        processPtr->exec(command, args);
        auto period = Common::Process::milli(GL_ProcTimeoutMilliSecs);
        if (processPtr->wait(period, GL_ProcMaxRetries) != Common::Process::ProcessStatus::FINISHED)
        {
            processPtr->kill();
            auto output = processPtr->output();
            std::stringstream ssTimeoutMessage;
            ssTimeoutMessage << "Timed out after "
                             << std::chrono::duration_cast<std::chrono::seconds>(period * GL_ProcMaxRetries).count()
                             << "s while running: '" << commandAndArgs << "'";
            throw SystemCommandsException(ssTimeoutMessage.str(), output);
        }

        auto output = processPtr->output();
        int exitCode = processPtr->exitCode();
        if (!exitcodes.empty())
        {
            if (std::find(exitcodes.begin(), exitcodes.end(), exitCode) == exitcodes.end())
            {
                std::stringstream expectedCodesString;
                expectedCodesString << "{";
                for (size_t i = 0; i < exitcodes.size(); ++i)
                {
                    if (i != 0)
                    {
                        expectedCodesString << ",";
                    }
                    expectedCodesString << exitcodes[i];
                }
                expectedCodesString << "}";

                throw SystemCommandsException(
                    " Expected exit code from range " + expectedCodesString.str() + ", Actual exit code is " +
                        std::to_string(exitCode) + ", Error message: " + Common::UtilityImpl::StrError(exitCode),
                    output);
            }

        }
        else if (exitCode != 0)
        {
            throw SystemCommandsException(
                 Common::UtilityImpl::StrError(exitCode),
                output);
        }
        return output;
    }

    void SystemCommands::tarDiagnoseFolder(const std::string& srcPath, const std::string& destPath) const
    {
        Common::UtilityImpl::FormattedTime formattedTime;

        std::cout << "Running tar on: " << srcPath << std::endl;

        std::string timestamp = formattedTime.currentTime();
        std::replace(timestamp.begin(), timestamp.end(), ' ', '_');
        std::string prefix = "sspl-diagnose_" + timestamp ;
        std::string tarfileName = prefix + ".tar.gz";

        std::string tarfile = Common::FileSystem::join(destPath, tarfileName);
        std::string prefixpath = Common::FileSystem::join(srcPath, prefix);
        std::string oldpath = Common::FileSystem::join(srcPath, DIAGNOSE_FOLDER);
        auto fs = fileSystem();
        fs->moveFile(oldpath,prefixpath);
        // use -C to move the current working directory to the temp folder holding Diagnose output
        std::string tarCommand =
            "tar -czf " + tarfile + " -C '" + srcPath + "' " + prefix;

        int ret = system(tarCommand.c_str());
        if (ret != 0)
        {
            throw std::invalid_argument("tar file command failed");
        }

        if (!fileSystem()->isFile(tarfile))
        {
            throw std::invalid_argument("tar file " + tarfile + " was not created");
        }
        std::cout << "Created tarfile: " << tarfileName << " in directory " << destPath << std::endl;
    }

    void SystemCommands::zipDiagnoseFolder(const std::string& srcPath, const std::string& destPath) const
    {

        LOGINFO("Running zip on: " << srcPath);
        std::string zipFileName = "sspl.zip";
        std::string zipfiletemp = Common::FileSystem::join(destPath, zipFileName + ".temp");
        std::string zipfile = Common::FileSystem::join(destPath, zipFileName);

        Common::ZipUtilities::ZipUtils zip;
        zip.produceZip(srcPath, zipfiletemp);

        try
        {
            Common::FileSystem::filePermissions()->chown(zipfiletemp, sophos::user(), sophos::group());
            Common::FileSystem::filePermissions()->chmod(zipfiletemp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

            fileSystem()->moveFile(zipfiletemp, zipfile);
        }
        catch (IFileSystemException& exception)
        {
            std::stringstream errormsg;
            errormsg << "Error creating zip file: " << exception.what();
            throw std::runtime_error(errormsg.str());
        }

        LOGINFO("Created tarfile: " << zipFileName << " in directory " << destPath);
    }
} // namespace diagnose
