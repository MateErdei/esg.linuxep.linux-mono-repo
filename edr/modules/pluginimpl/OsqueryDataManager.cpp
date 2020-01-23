/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OsqueryDataManager.h"

#include "ApplicationPaths.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>

void OsqueryDataManager::cleanUpOsqueryLogs()
{
    std::string logPath = Plugin::osQueryResultsLogPath();
    auto* ifileSystem = Common::FileSystem::fileSystem();
    try
    {
        off_t size = ifileSystem->fileSize(logPath);

        if (size > MAX_LOGFILE_SIZE)
        {
            LOGDEBUG("Rotating osquery logs");
            std::string fileToDelete = logPath + ".10";

            if (ifileSystem->isFile(fileToDelete))
            {
                LOGINFO("Log limit reached : Deleting oldest osquery log file");
                ifileSystem->removeFile(fileToDelete);
            }

            rotateFiles(logPath, 9);

            ifileSystem->moveFile(logPath, logPath + ".1");
        }
        removeOldWarningFiles();
    }
    catch (Common::FileSystem::IFileSystemException& ex)
    {
        LOGERROR("Failed to clean up old osquery files with error: " << ex.what());
    }
}
void OsqueryDataManager::removeOldWarningFiles()
{
    LOGDEBUG("Checking for osquery INFO/WARNING files");
    auto* ifileSystem = Common::FileSystem::fileSystem();
    std::vector<std::string> files = ifileSystem->listFiles(Plugin::osQueryLogPath());
    std::vector<std::string> warningFiles;
    std::vector<std::string> infoFiles;
    try
    {
        for (const auto file : files)
        {
            std::string filename = Common::FileSystem::basename(file);
            if (filename.find("osqueryd.WARNING.") != std::string::npos)
            {
                warningFiles.push_back(file);
            }
            else if (filename.find("osqueryd.INFO.") != std::string::npos)
            {
                infoFiles.push_back(file);
            }
        }

        if (infoFiles.size() > FILE_LIMIT)
        {
            std::sort(infoFiles.begin(), infoFiles.end());

            int infoFilesToDelete = infoFiles.size() - FILE_LIMIT;

            for (int i = 0; i < infoFilesToDelete; i++)
            {
                ifileSystem->removeFile(infoFiles[i]);
                LOGINFO("Removed old osquery INFO file: " << Common::FileSystem::basename(infoFiles[i]));
            }
        }

        if (warningFiles.size() > FILE_LIMIT)
        {
            std::sort(warningFiles.begin(), warningFiles.end());

            int warningFilesToDelete = warningFiles.size() - FILE_LIMIT;
            for (int i = 0; i < warningFilesToDelete; i++)
            {
                ifileSystem->removeFile(warningFiles[i]);
                LOGINFO("Removed old osquery WARNING file: " << Common::FileSystem::basename(warningFiles[i]));
            }
        }
    }
    catch (Common::FileSystem::IFileSystemException& ex)
    {
        LOGERROR("Failed to clean up old osquery WARNING/INFO files with error: " << ex.what());
    }
}

void OsqueryDataManager::rotateFiles(std::string path, int limit)
{
    auto* ifileSystem = Common::FileSystem::fileSystem();
    int iterator = limit;
    try
    {
        while (iterator > 0)
        {
            std::string oldExtension = "." + std::to_string(iterator);
            std::string fileToIncrement = path + oldExtension;

            if (ifileSystem->isFile(fileToIncrement))
            {
                std::string newExtension = "." + std::to_string(iterator + 1);
                std::string fileDestination = path + newExtension;
                ifileSystem->moveFile(fileToIncrement, fileDestination);
            }

            iterator -= 1;
        }
    }
    catch (Common::FileSystem::IFileSystemException& ex)
    {
        LOGERROR("Failed to rotate files with error: " << ex.what());
    }
}