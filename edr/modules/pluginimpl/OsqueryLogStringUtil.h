/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <string>
#include <optional>

class OsqueryLogStringUtil
{
public:
    static std::optional<std::string> processOsqueryLogLineForScheduledQueries(const std::string& logLine);
    static bool isGenericLogLine(const std::string& logLine);
};


