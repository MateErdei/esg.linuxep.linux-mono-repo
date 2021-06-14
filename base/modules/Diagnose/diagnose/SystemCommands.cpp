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

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <sys/stat.h>
#include <zip.h>
#include <fstream>
#include <zlib.h>

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
        const std::string& filename) const
    {
        Path filePath = Common::FileSystem::join(m_destination, filename);
        LOGINFO("Output file path: " << filePath);

        try
        {
            std::string exePath = getExecutablePath(command);
            auto output = runCommandOutputToString(exePath, arguments);
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
            LOGINFO("Running command: '" << command << "' failed  to complete with: " << e.what());

            std::stringstream message;
            message << e.output() << "***End Of Command Output***" << std::endl
                    << "Running command failed to complete with error: " << e.what();
            fileSystem()->writeFile(filePath, message.str());
        }

        return EXIT_FAILURE;
    }

    std::string SystemCommands::runCommandOutputToString(const std::string& command, std::vector<std::string> args)
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
        if (exitCode != 0)
        {
            throw SystemCommandsException(
                "Process execution returned non-zero exit code, 'Exit Code: " + Common::UtilityImpl::StrError(exitCode),
                output);
        }
        return output;
    }

    std::string SystemCommands::getExecutablePath(const std::string& executableName) const
    {
        std::vector<std::string> folderLocations = { "/usr/bin", "/bin", "/usr/local/bin", "/sbin", "/usr/sbin" };
        for (const auto& folder : folderLocations)
        {
            Path path = Common::FileSystem::join(folder, executableName);
            if (fileSystem()->isExecutable(path))
            {
                return path;
            }
        }
        throw std::invalid_argument("Executable " + executableName + " is not installed.");
    }

    void SystemCommands::tarDiagnoseFolder(const std::string& srcPath, const std::string& destPath) const
    {
        Common::UtilityImpl::FormattedTime m_formattedTime;

        std::cout << "Running tar on: " << srcPath << std::endl;

        std::string timestamp = m_formattedTime.currentTime();
        std::replace(timestamp.begin(), timestamp.end(), ' ', '_');
        std::string tarfileName = "sspl-diagnose_" + timestamp + ".tar.gz";

        std::string tarfile = Common::FileSystem::join(destPath, tarfileName);

        std::string tarCommand =
            "tar -czf " + tarfile + " -C '" + srcPath + "' " + PLUGIN_FOLDER + " " + BASE_FOLDER + " " + SYSTEM_FOLDER;

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

    void SystemCommands::walkDirectoryTree(std::vector<std::string>& pathCollection, const std::string& root) const
    {
        auto fs = Common::FileSystem::fileSystem();
        std::vector<std::string> files;
        try
        {
            files = fs->listFiles(root);
        }
        catch(IFileSystemException& exception)
        {
            std::cout << "Failed to get list of files for :'" << root << "'" << std::endl;
            return;
        }

        for (auto& file : files)
        {
            pathCollection.push_back(file);
        }

        std::vector<std::string> directories;
        try
        {
            directories = fs->listDirectories(root);
        }
        catch(IFileSystemException& exception)
        {
            std::cout << "Failed to get list of directories for :'" << root << "'" << std::endl;
            return;
        }

        for (auto& directory : directories)
        {
            walkDirectoryTree(pathCollection, directory);
        }
    }

    void SystemCommands::produceZip(const std::string& srcPath, const std::string& destPath) const
    {
        zipFile zf = zipOpen(std::string(destPath.begin(), destPath.end()).c_str(), APPEND_STATUS_CREATE);
        if (zf == NULL)
            return;

        bool error = false;
        auto fs = Common::FileSystem::fileSystem();

        std::vector<std::string> filesToZip;
        walkDirectoryTree(filesToZip, srcPath);

        for (auto& path : filesToZip)
        {
            if (fs->isFile(path))
            {

                std::fstream file(path.c_str(), std::ios::binary | std::ios::in);
                if (file.is_open())
                {
                    file.seekg(0, std::ios::end);
                    long size = file.tellg();
                    file.seekg(0, std::ios::beg);
                    std::cout << size  << std::endl;
                    std::vector<char> buffer(size);
                    if (size == 0 || file.read(&buffer[0], size))
                    {
                        zip_fileinfo zfi;
                        std::string fileName = Common::FileSystem::basename(path);

                        fileName = path.substr(srcPath.size());

                        if (0 == zipOpenNewFileInZip(zf, std::string(fileName.begin(), fileName.end()).c_str(), &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION))
                        {
                            if (zipWriteInFileInZip(zf, size == 0 ? "" : &buffer[0], size))
                                error = true;

                            if (zipCloseFileInZip(zf))
                                error = true;

                            file.close();
                            continue;
                        }
                    }
                    file.close();
                }
                error = true;
            }
        }

        if (zipClose(zf, NULL))
            return ;

        if (error)
            return ;

    }

    void SystemCommands::zipDiagnoseFolder(const std::string& srcPath, const std::string& destPath) const
    {

        std::cout << "Running zip on: " << srcPath << std::endl;
        std::string zipFileName = "sspl.zip";
        std::string zipfiletemp = Common::FileSystem::join(destPath, zipFileName + ".temp");
        std::string zipfile = Common::FileSystem::join(destPath, zipFileName);

        produceZip(srcPath, zipfiletemp);

        Common::FileSystem::filePermissions()->chown(zipfiletemp, sophos::user(), sophos::group());
        Common::FileSystem::filePermissions()->chmod(zipfiletemp, S_IRUSR | S_IWUSR | S_IRGRP);
        try
        {
            fileSystem()->moveFile(zipfiletemp, zipfile);
        }
        catch(IFileSystemException& exception)
        {
            throw std::invalid_argument("zip file " + zipfile + " was not created, error" + exception.what());
        }

        std::cout << "Created tarfile: " << zipFileName << " in directory " << destPath << std::endl;
    }
} // namespace diagnose
