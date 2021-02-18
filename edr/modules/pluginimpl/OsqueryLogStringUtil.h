/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <string>

class OsqueryLogStringUtil
{
public:
    static std::tuple<bool, std::string> processOsqueryLogLineForScheduledQueries(std::string& logLine);
    static bool isGenericLogLine(std::string& logLine);
};


