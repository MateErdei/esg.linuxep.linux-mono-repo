/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OsqueryDataManager.h"

#include "Logger.h"
#include "ApplicationPaths.h"

#include <Common/FileSystem/IFileSystem.h>


void OsqueryDataManager::CleanUpOsqueryLogs()
{
    std::string logPath = Plugin::osQueryResultsLogPath();
    auto* ifileSystem = Common::FileSystem::fileSystem();
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

        ifileSystem->moveFile(logPath,logPath + ".1");
    }
    removeOldWarningFiles();

}
void OsqueryDataManager::removeOldWarningFiles()
{
    LOGDEBUG("Checking for osquery INFO/WARNING files");
    auto* ifileSystem = Common::FileSystem::fileSystem();
    std::vector<std::string> files = ifileSystem->listFiles(Plugin::osQueryLogPath());
    std::vector<std::string> warningFiles;
    std::vector<std::string> InfoFiles;
    for (const auto file :files)
    {
        std::string filename = Common::FileSystem::basename(file);
        if (filename.find("osqueryd.WARNING.") != std::string::npos)
        {
            warningFiles.push_back(file);
        }
        else if (filename.find("osqueryd.INFO.") != std::string::npos)
        {
            InfoFiles.push_back(file);
        }
    }

    if (InfoFiles.size() > 10)
    {
        std::sort(InfoFiles.begin(),InfoFiles.end());
        int iterator = InfoFiles.size();
        while(iterator > 10)
        {
            ifileSystem->removeFile(InfoFiles.back());
            InfoFiles.pop_back();
            iterator = iterator - 1;
        }
        LOGINFO("Removed old osquery INFO files");
    }

    if (warningFiles.size() > 10)
    {
        std::sort(warningFiles.begin(),warningFiles.end());
        int iterator = warningFiles.size();
        while(iterator > 10)
        {
            ifileSystem->removeFile(warningFiles.back());
            warningFiles.pop_back();
            iterator = iterator - 1;
        }
        LOGINFO("Removed old osquery WARNING files");
    }
}

void OsqueryDataManager::rotateFiles(std::string path, int limit)
{
    auto* ifileSystem = Common::FileSystem::fileSystem();
    int iterator = limit;
    while (iterator > 0)
    {
        std::string oldExtension = "." + std::to_string(iterator);
        std::string fileToIncrement = path + oldExtension;

        if (ifileSystem->isFile(fileToIncrement))
        {
            std::string newExtension = "." + std::to_string(iterator + 1);
            std::string fileDestination = path + newExtension;
            ifileSystem->moveFile(fileToIncrement,fileDestination);
        }

        iterator -= 1;
    }
}

