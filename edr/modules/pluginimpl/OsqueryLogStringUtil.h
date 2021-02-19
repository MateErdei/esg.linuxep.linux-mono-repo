/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <string>
#include <optional>

class OsqueryLogStringUtil
{
public:
    static std::optional<std::string> processOsqueryLogLineForScheduledQueries(std::string& logLine);
    static bool isGenericLogLine(std::string& logLine);
};


