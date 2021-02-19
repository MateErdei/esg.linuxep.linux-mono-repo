/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OsqueryLogStringUtil.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>


std::optional<std::string> OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(std::string& logLine)
{
    std::string lineToFind= "Executing scheduled query ";
    if (Common::UtilityImpl::StringUtils::isSubstring(logLine, lineToFind))
    {
        std::vector<std::string> items = Common::UtilityImpl::StringUtils::splitString(logLine, lineToFind);
        if(items.size() < 2)
        {
            return {};
        }

        std::vector<std::string> timeString = Common::UtilityImpl::StringUtils::splitString(items[0], " ");
        if(timeString.size() < 2)
        {
            return {};
        }

        std::string time = timeString[1];

        std::vector<std::string> nameString = Common::UtilityImpl::StringUtils::splitString(items[1], ":");
        if(timeString.empty())
        {
            return {};
        }

        std::string queryName = nameString[0];

        if(nameString.empty() || time.empty())
        {
            return {};
        }

        std::stringstream lineReturned ;
        lineReturned << "Executing query: " << queryName << " at: " << time ;
        return  lineReturned.str();
    }
    return {};
}

bool OsqueryLogStringUtil::isGenericLogLine(std::string& logLine)
{
    std::vector<std::string> interestingLines{"Error executing"};

    for(auto& interestingLine : interestingLines)
    {
        if(Common::UtilityImpl::StringUtils::isSubstring(logLine, interestingLine))
        {
            return true;
        }
    }
    return false;
}


