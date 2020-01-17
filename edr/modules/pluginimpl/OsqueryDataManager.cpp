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

        int iterator = 9;
        while (iterator > 0)
        {
            std::string oldExtension = "." + std::to_string(iterator);
            std::string fileToIncrement = Common::FileSystem::join(logPath,oldExtension);

            if (ifileSystem->isFile(fileToIncrement))
            {
                std::string newExtension = "." + std::to_string(iterator + 1);
                std::string fileDestination = Common::FileSystem::join(logPath,newExtension);
                ifileSystem->moveFile(fileToIncrement,fileDestination);
            }

            iterator -= 1;
        }
    }
}

