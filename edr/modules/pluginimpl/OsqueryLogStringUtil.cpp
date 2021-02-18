/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OsqueryLogStringUtil.h"

#include "TelemetryConsts.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>


std::tuple<bool, std::string> OsqueryLogStringUtil::processOsqueryLogLineForScheduledQueries(std::string& logLine)
{
    std::string lineToFind= "Executing scheduled query ";
    if (Common::UtilityImpl::StringUtils::isSubstring(logLine, lineToFind))
    {
        std::vector<std::string> words = Common::UtilityImpl::StringUtils::splitString(logLine, lineToFind);
        if(words.size() < 2)
        {
            return std::make_tuple(false, "");
        }

        std::vector<std::string> timeString = Common::UtilityImpl::StringUtils::splitString(words[0], " ");
        if(timeString.size() < 2)
        {
            return std::make_tuple(false, "");
        }

        std::string time = timeString[1];

        std::vector<std::string> nameString = Common::UtilityImpl::StringUtils::splitString(words[1], ":");
        if(timeString.empty())
        {
            return std::make_tuple(false, "");
        }

        std::string queryName = nameString[0];

        if(nameString.empty() || time.empty())
        {
            return std::make_tuple(false, "");
        }

        std::stringstream lineReturned ;
        lineReturned << "Executing query: " << queryName << " at: " << time ;
        return std::make_tuple(true, lineReturned.str());
    }
    return std::make_tuple(false, "");
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


