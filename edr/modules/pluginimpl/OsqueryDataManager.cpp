/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OsqueryDataManager.h"

#include "Logger.h"
#include "ApplicationPaths.h"

#include <Common/FileSystem/IFileSystem.h>


void OsqueryDataManager::rotateOsqueryLogs()
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

