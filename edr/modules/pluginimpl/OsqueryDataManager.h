/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <future>

class OsqueryDataManager
{
public:
    void cleanUpOsqueryLogs();
    void removeOldWarningFiles();
    void rotateFiles(std::string path, int limit);

private:
    const unsigned int MAX_LOGFILE_SIZE = 1024 * 1024;
    const unsigned int FILE_LIMIT = 10;

};

