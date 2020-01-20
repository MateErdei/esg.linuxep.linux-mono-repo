/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <future>

class OsqueryDataManager
{
public:
    void CleanUpOsqueryLogs();
    void removeOldWarningFiles();
    void rotateFiles(std::string path, int limit);

private:
    int MAX_LOGFILE_SIZE = 1024 * 1024;

};

